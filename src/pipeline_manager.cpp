#include "pipeline_manager.h"
#include "meta_data.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string>
#include <list>

using std::list;

dng_md_t dng_all_md;

pipeline_manager::pipeline_manager()
{
    stat_addr = new statistic_info_t;
    if (stat_addr == nullptr)
    {
        spdlog::error("alloc memory for statistc_info fail");
    }
}

pipeline_manager::~pipeline_manager()
{
    for (vector<hw_base*>::iterator it = modules_list.begin(); it != modules_list.end(); it++)
    {
        delete (*it);
    }
    //statistic_info_t
    if (stat_addr != nullptr)
    {
        delete stat_addr;
    }
}
void pipeline_manager::register_module(hw_base* hw_module)
{
    modules_list.push_back(hw_module);
}
void pipeline_manager::init()
{
    size_t module_cnt = modules_list.size();
    for (size_t i = 0; i < module_cnt; i++)
    {
        modules_list.at(i)->init();
    }
}


static bool xmlHasChildElementByName(xmlNodePtr parent, const xmlChar* tagName, xmlChar** inst_name)
{
    xmlNodePtr child = parent->children;
    xmlChar tarName[10] = "inst_name";
    while (child != NULL)
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
    if (ctxt == NULL) {
        spdlog::error("Failed to allocate parser context");
        return;
    }
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        spdlog::error("Failed to parse {}", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    if (ctxt->valid == 0)
    {
        spdlog::error("Failed to validate {}", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    if (rootNode == NULL)
    {
        spdlog::error("empty document\n");
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    spdlog::info("root node name: {0}, type:{1}", rootNode->name, rootNode->type); //pipeline_config
    xmlNodePtr component = rootNode->children;
    while (component != NULL)
    {
        spdlog::info("component name {0}, type:{1}", component->name, component->type); //component

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
        while (detail_node != NULL) 
        {
            if (xmlChildElementCount(detail_node) == 0) { //leaf node
                xmlChar* text = xmlNodeGetContent(detail_node);
                delete_space_in_string(text);
                spdlog::info("tag name:{0} text:{1}", detail_node->name, text);
                size_t cfg_len = module_array->at(i)->cfgList.size();
                size_t j = 0;
                for (; j < cfg_len; j++)
                {
                    if (xmlStrcmp(detail_node->name, (const xmlChar*)module_array->at(i)->cfgList[j].tagName) == 0)
                    {
                        int res_bool;
                        int str_len;
                        int dst_len;
                        int res_int;
                        long int res_li;
                        int m;
                        const char s[2] = ",";
                        char *token;
                        char *next_token = NULL;
                        switch (module_array->at(i)->cfgList[j].type)
                        {
                        case BOOL_T:
                            res_bool = std::stoi((char*)text);
                            if (res_bool == 0) {
                                *(bool*)module_array->at(i)->cfgList[j].targetAddr = false;
                            }
                            else {
                                *(bool*)module_array->at(i)->cfgList[j].targetAddr = true;
                            }
                            break;
                        case STRING:
                            str_len = xmlStrlen(text);
                            dst_len = (int)module_array->at(i)->cfgList[j].max_len-1;
                            if (str_len > dst_len)
                            {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->cfgList[j].targetAddr, dst_len, text, dst_len);
#else
                                memcpy(module_array->at(i)->cfgList[j].targetAddr, text, dst_len);
#endif
                                ((char*)(module_array->at(i)->cfgList[j].targetAddr))[dst_len] = '\0';
                            }
                            else {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->cfgList[j].targetAddr, str_len, text, str_len);
#else
                                memcpy(module_array->at(i)->cfgList[j].targetAddr, text, str_len);
#endif
                                ((char*)(module_array->at(i)->cfgList[j].targetAddr))[str_len] = '\0';
                            }
                            break;
                        case INT_32:
                            res_int = std::stoi((char*)text);
                            *(int*)module_array->at(i)->cfgList[j].targetAddr = res_int;
                            break;
                        case UINT_32:
                            res_li = std::stol((char*)text);
                            *(uint32_t*)module_array->at(i)->cfgList[j].targetAddr = (uint32_t)res_li;
                            break;
                        case VECT_INT32:
#ifdef _MSC_VER
                            token = strtok_s((char*)text, s, &next_token);
#else
                            token = strtok((char*)text, s);
#endif
                            m = 0;
                            while (token != NULL && m< (module_array->at(i)->cfgList[j].max_len)) {
                                int vect_int = std::stoi(token);
                                ((vector<int32_t>*)(module_array->at(i)->cfgList[j].targetAddr))->push_back(vect_int);
                                m++;
#ifdef _MSC_VER
                                token = strtok_s(NULL, s, &next_token);
#else
                                token = strtok(NULL, s);
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
                            while (token != NULL && m < (module_array->at(i)->cfgList[j].max_len)) {
                                long int vect_lint = std::stol(token);
                                ((vector<uint32_t>*)(module_array->at(i)->cfgList[j].targetAddr))->push_back((uint32_t)vect_lint);
                                m++;
#ifdef _MSC_VER
                                token = strtok_s(NULL, s, &next_token);
#else
                                token = strtok(NULL, s);
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

void pipeline_manager::read_xml_cfg(char* xmlFileName)
{
    LIBXML_TEST_VERSION

    exampleFunc(xmlFileName, &this->modules_list);

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
    size_t md_size = modules_list.size();
    list<hw_base*> not_run_list(modules_list.begin(), modules_list.end());
    
    for (size_t i = 0; i < md_size; i++)
    {
        modules_list.at(i)->reset_hw_cnt_of_outport();
    }

    size_t cycle_time = 0;
    while (not_run_list.size() != 0)
    {
        for (list<hw_base*>::iterator it = not_run_list.begin(); it != not_run_list.end(); it++)
        {
            if ((*it)->prepare_input())
            {
                spdlog::info("run module {}", (*it)->name);
                (*it)->hw_run(stat_out, frame_cnt);
                not_run_list.remove(*it);
                break;
            }
        }

        cycle_time++;
        if (cycle_time > 2 * md_size)
        {
            spdlog::error("can't find module to run");
            exit(1);
        }
    }

    for (size_t i = 0; i < md_size; i++)
    {
        modules_list.at(i)->release_output_memory();
    }
}