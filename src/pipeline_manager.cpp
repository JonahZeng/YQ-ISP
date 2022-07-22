#include "pipeline_manager.h"
#include "meta_data.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include <string>
#include <list>
#include <typeinfo>
#include <algorithm>
#include "file_read.h"

using std::list;
using std::vector;

pipeline_manager* pipeline_manager::g_pipe_manager = nullptr;

pipeline_manager::pipeline_manager():hw_list(), cfg_file_name()
{
    stat_addr = new statistic_info_t;
    global_ref_out = new global_ref_out_t;
    if (stat_addr == nullptr || global_ref_out == nullptr)
    {
        log_error("alloc memory for statistc_info/global ref fail\n");
    }

    global_ref_out->dng_meta_data.meta_data_valid = false;
}

pipeline_manager::~pipeline_manager()
{
    for (vector<hw_base*>::iterator it = hw_list.begin(); it != hw_list.end(); it++)
    {
        delete (*it);
    }
    //statistic_info_t
    if (stat_addr != nullptr)
    {
        delete stat_addr;
    }
    if (global_ref_out != nullptr)
    {
        delete global_ref_out;
    }
}

void pipeline_manager::register_hw_module(hw_base* hw_module)
{
    if(std::find(hw_list.begin(), hw_list.end(), hw_module) == hw_list.end())
    {
        hw_list.push_back(hw_module);
    }
}

void pipeline_manager::init()
{
    size_t hw_cnt = hw_list.size();
    for (size_t i = 0; i < hw_cnt; i++)
    {
        hw_list.at(i)->hw_init();
    }
}


static bool xmlHasChildElementByName(xmlNodePtr parent, const xmlChar* tagName, xmlChar** inst_name)
{
    xmlNodePtr child = parent->children;
    xmlChar tarName[10] = "inst_name";
    while (child != nullptr)
    {
        if (xmlStrcmp(child->name, tarName) == 0)
        {
            *inst_name = xmlNodeGetContent(child);
            return true;
        }
        child = child->next;
    }
    return false;
}

static void delete_space_in_string(xmlChar* str)
{
    int str_len = xmlStrlen(str);
    int i = 0, j = 0;
    for (; j < str_len; j++)
    {
        if (str[j] == ' ')
        {
            while (str[j] == ' ' && j < str_len)
            {
                j++;
            }
            str[i] = str[j];
            i++;
        }
        else {
            str[i] = str[j];
            i++;
        }
    }
    str[i] = '\0';
}

static void exampleFunc(const char *filename, vector<hw_base*>* module_array) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;

    ctxt = xmlNewParserCtxt();
    if (ctxt == nullptr) {
        log_error("Failed to allocate parser context\n");
        return;
    }
    doc = xmlCtxtReadFile(ctxt, filename, nullptr, XML_PARSE_NOBLANKS);

    if (doc == nullptr) {
        log_error("Failed to parse %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    if (ctxt->valid == 0)
    {
        log_error("Failed to validate %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    if (rootNode == nullptr)
    {
        log_error("empty document\n");
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    log_info("root node name: %s, type:%d\n", rootNode->name, rootNode->type); //pipeline_config
    xmlNodePtr component = rootNode->children;
    while (component != nullptr)
    {
        log_info("component name %s, type:%d\n", component->name, component->type); //component

        xmlChar* inst_name;
        xmlChar tagName[10] = "inst_name";
        bool has_inst_name = xmlHasChildElementByName(component, tagName, &inst_name);
        if (has_inst_name == false)
        {
            component = component->next;
            continue;
        }

        size_t module_size = module_array->size();
        size_t i = 0;
        for (; i < module_size; i++)
        {
            if (xmlStrcmp((const xmlChar*)module_array->at(i)->name, inst_name) == 0)
            {
                break;
            }
        }
        xmlFree(inst_name);
        if (i >= module_size)
        {
            component = component->next;
            continue;
        }

        xmlNodePtr detail_node = component->children;
        while (detail_node != nullptr) 
        {
            if (xmlChildElementCount(detail_node) == 0) { //leaf node
                xmlChar* text = xmlNodeGetContent(detail_node);
                delete_space_in_string(text);
                log_info("tag name:%s text:%s\n", detail_node->name, text);
                size_t cfg_len = module_array->at(i)->hwCfgList.size();
                size_t j = 0;
                for (; j < cfg_len; j++)
                {
                    if (xmlStrcmp(detail_node->name, (const xmlChar*)module_array->at(i)->hwCfgList[j].tagName) == 0)
                    {
                        int res_bool;
                        int str_len;
                        int dst_len;
                        int res_int;
                        long int res_li;
                        int m;
                        const char s[2] = ",";
                        char *token;
                        char *next_token = nullptr;
                        switch (module_array->at(i)->hwCfgList[j].type)
                        {
                        case BOOL_T:
                            res_bool = std::stoi((char*)text);
                            if (res_bool == 0) {
                                *(bool*)module_array->at(i)->hwCfgList[j].targetAddr = false;
                            }
                            else {
                                *(bool*)module_array->at(i)->hwCfgList[j].targetAddr = true;
                            }
                            break;
                        case STRING:
                            str_len = xmlStrlen(text);
                            dst_len = (int)module_array->at(i)->hwCfgList[j].max_len-1;
                            if (str_len > dst_len)
                            {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->hwCfgList[j].targetAddr, dst_len, text, dst_len);
#else
                                memcpy(module_array->at(i)->hwCfgList[j].targetAddr, text, dst_len);
#endif
                                ((char*)(module_array->at(i)->hwCfgList[j].targetAddr))[dst_len] = '\0';
                            }
                            else {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->hwCfgList[j].targetAddr, str_len, text, str_len);
#else
                                memcpy(module_array->at(i)->hwCfgList[j].targetAddr, text, str_len);
#endif
                                ((char*)(module_array->at(i)->hwCfgList[j].targetAddr))[str_len] = '\0';
                            }
                            break;
                        case INT_32:
                            res_int = std::stoi((char*)text);
                            *(int*)module_array->at(i)->hwCfgList[j].targetAddr = res_int;
                            break;
                        case UINT_32:
                            res_li = std::stol((char*)text);
                            *(uint32_t*)module_array->at(i)->hwCfgList[j].targetAddr = (uint32_t)res_li;
                            break;
                        case VECT_INT32:
#ifdef _MSC_VER
                            token = strtok_s((char*)text, s, &next_token);
#else
                            token = strtok((char*)text, s);
#endif
                            m = 0;
                            while (token != nullptr && m< (module_array->at(i)->hwCfgList[j].max_len)) {
                                int vect_int = std::stoi(token);
                                if (m >= ((vector<int32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->size())
                                    ((vector<int32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->push_back(vect_int);
                                else
                                    ((vector<int32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->at(m) = vect_int;
                                m++;
#ifdef _MSC_VER
                                token = strtok_s(nullptr, s, &next_token);
#else
                                token = strtok(nullptr, s);
#endif
                            }
                            break;
                        case VECT_UINT32:
#ifdef _MSC_VER
                            token = strtok_s((char*)text, s, &next_token);
#else
                            token = strtok((char*)text, s);
#endif
                            m = 0;
                            while (token != nullptr && m < (module_array->at(i)->hwCfgList[j].max_len)) {
                                long int vect_lint = std::stol(token);
                                if (m >= ((vector<int32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->size())
                                    ((vector<uint32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->push_back((uint32_t)vect_lint);
                                else
                                    ((vector<uint32_t>*)(module_array->at(i)->hwCfgList[j].targetAddr))->at(m) = (uint32_t)vect_lint;
                                m++;
#ifdef _MSC_VER
                                token = strtok_s(nullptr, s, &next_token);
#else
                                token = strtok(nullptr, s);
#endif
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                }
                
                xmlFree(text);
            }
            detail_node = detail_node->next;
        }
        component = component->next;
    }
    /*
    XML_ELEMENT_NODE=		1,
    XML_ATTRIBUTE_NODE=		2,
    XML_TEXT_NODE=		3,
    XML_CDATA_SECTION_NODE=	4,
    XML_ENTITY_REF_NODE=	5,
    XML_ENTITY_NODE=		6,
    XML_PI_NODE=		7,
    XML_COMMENT_NODE=		8,
    XML_DOCUMENT_NODE=		9,
    XML_DOCUMENT_TYPE_NODE=	10,
    XML_DOCUMENT_FRAG_NODE=	11,
    XML_NOTATION_NODE=		12,
    XML_HTML_DOCUMENT_NODE=	13,
    XML_DTD_NODE=		14,
    XML_ELEMENT_DECL=		15,
    XML_ATTRIBUTE_DECL=		16,
    XML_ENTITY_DECL=		17,
    XML_NAMESPACE_DECL=		18,
    XML_XINCLUDE_START=		19,
    XML_XINCLUDE_END=		20,
    XML_DOCB_DOCUMENT_NODE=	21
    */

    xmlFreeParserCtxt(ctxt);
}

void pipeline_manager::read_xml_cfg()
{
    LIBXML_TEST_VERSION

    exampleFunc(cfg_file_name.c_str(), &this->hw_list);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
}

void pipeline_manager::run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    size_t hw_size = hw_list.size();
    list<hw_base*> not_run_list(hw_list.begin(), hw_list.end());
    
    for (size_t i = 0; i < hw_size; i++)
    {
        hw_list.at(i)->reset_hw_cnt_of_outport();
    }

    size_t cycle_time = 0;
    while (not_run_list.size() != 0)
    {
        for (list<hw_base*>::iterator it = not_run_list.begin(); it != not_run_list.end(); it++)
        {
            if ((*it)->prepare_input())
            {
                log_info("run module %s\n", (*it)->name);
                (*it)->hw_run(stat_out, frame_cnt);
                not_run_list.remove(*it);
                break;
            }
        }

        cycle_time++;
        if (cycle_time > 2 * hw_size)
        {
            log_error("can't find module to run\n");
            exit(1);
        }
    }

    for (size_t i = 0; i < hw_size; i++)
    {
        hw_base* m = hw_list.at(i);
        if (typeid(*m) == typeid(file_read))
        {
            if (frame_cnt == frames - 1)
            {
                m->release_output_memory();
            }
        }
        else 
        {
            m->release_output_memory();
        }
    }
}

void pipeline_manager::connect_port(hw_base* pre_hw, uint32_t out_port, hw_base* next_hw, uint32_t in_port)
{
    if(std::find(hw_list.begin(), hw_list.end(), pre_hw) == hw_list.end() || 
        std::find(hw_list.begin(), hw_list.end(), next_hw) == hw_list.end())
    {
        log_warning("%s or %s not in hw_list\n", pre_hw->name, next_hw->name);
    }
    if (out_port >= pre_hw->outpins || in_port >= next_hw->inpins)
    {
        log_error("port out of range\n");
        exit(EXIT_FAILURE);
    }
    else {
        pre_hw->next_hw_of_outport[out_port].push_back(next_hw);
        pre_hw->next_hw_cnt_of_outport[out_port]++;
        next_hw->previous_hw[in_port] = pre_hw;
        next_hw->outport_of_previous_hw[in_port] = out_port;
    }
}

pipeline_manager* pipeline_manager::get_current_pipe_manager()
{
    return g_pipe_manager;
}

void pipeline_manager::set_current_pipe_manager(pipeline_manager* pipe_manager)
{
    g_pipe_manager = pipe_manager;
}