#include "fw_manager.h"
#include "fe_firmware.h"
#include "libxml/parser.h"
#include "libxml/tree.h"

fw_manager::fw_manager(uint32_t inpins, uint32_t outpins, const char *inst_name) : hw_base(inpins, outpins, inst_name), fw_list()
{
    bypass = 0;
}

void fw_manager::hw_run(statistic_info_t *stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);

    for (uint32_t i = 0; i < inpins; i++)
    {
        data_buffer *input_raw = in[i];
        bayer_type_t bayer_pattern = input_raw->bayer_pattern;

        uint32_t xsize = input_raw->width;
        uint32_t ysize = input_raw->height;

        char out_name[64] = {0};
        sprintf(out_name, "%s_out%d", name, i);

        data_buffer *output_data = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, out_name);

        out[i] = output_data;

        uint16_t *out_ptr = output_data->data_ptr;

        for (uint32_t sz = 0; sz < xsize * ysize; sz++)
        {
            out_ptr[sz] = input_raw->data_ptr[sz];
        }
    }

    size_t reg_len = sizeof(fe_module_reg_t) / sizeof(uint16_t);
    if (reg_len * sizeof(uint16_t) < sizeof(fe_module_reg_t))
    {
        reg_len += 1;
    }
    data_buffer *output_reg = new data_buffer((uint32_t)reg_len, 1, DATA_TYPE_RAW, BAYER_UNSUPPORT, "fw_manager_out_reg");
    fe_module_reg_t *reg_ptr = (fe_module_reg_t *)output_reg->data_ptr;

    out[outpins - 1] = output_reg;

    for(auto md: fw_list)
    {
        md->fw_exec(stat_out, global_ref_out, frame_cnt, (void*)reg_ptr);
    }

    this->write_pic = false;
    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void fw_manager::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass", UINT_32, &this->bypass}};
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    for(auto md: fw_list)
    {
        md->fw_init();
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

fw_manager::~fw_manager()
{
    log_info("%s module deinit start\n", __FUNCTION__);

    log_info("%s module deinit end\n", __FUNCTION__);
}

void fw_manager::regsiter_fw_modules(fw_base *fw_md)
{
    fw_list.push_back(fw_md);
}

void fw_manager::set_glb_ref(global_ref_out_t *global_ref_out)
{
    this->global_ref_out = global_ref_out;
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


static void exampleFunc(const char *filename, std::vector<fw_base*>* module_array) {
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
                size_t cfg_len = module_array->at(i)->fwCfgList.size();
                size_t j = 0;
                for (; j < cfg_len; j++)
                {
                    if (xmlStrcmp(detail_node->name, (const xmlChar*)module_array->at(i)->fwCfgList[j].tagName) == 0)
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
                        switch (module_array->at(i)->fwCfgList[j].type)
                        {
                        case BOOL_T:
                            res_bool = std::stoi((char*)text);
                            if (res_bool == 0) {
                                *(bool*)module_array->at(i)->fwCfgList[j].targetAddr = false;
                            }
                            else {
                                *(bool*)module_array->at(i)->fwCfgList[j].targetAddr = true;
                            }
                            break;
                        case STRING:
                            str_len = xmlStrlen(text);
                            dst_len = (int)module_array->at(i)->fwCfgList[j].max_len-1;
                            if (str_len > dst_len)
                            {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->fwCfgList[j].targetAddr, dst_len, text, dst_len);
#else
                                memcpy(module_array->at(i)->hwCfgList[j].targetAddr, text, dst_len);
#endif
                                ((char*)(module_array->at(i)->fwCfgList[j].targetAddr))[dst_len] = '\0';
                            }
                            else {
#ifdef _MSC_VER
                                memcpy_s(module_array->at(i)->fwCfgList[j].targetAddr, str_len, text, str_len);
#else
                                memcpy(module_array->at(i)->hwCfgList[j].targetAddr, text, str_len);
#endif
                                ((char*)(module_array->at(i)->fwCfgList[j].targetAddr))[str_len] = '\0';
                            }
                            break;
                        case INT_32:
                            res_int = std::stoi((char*)text);
                            *(int*)module_array->at(i)->fwCfgList[j].targetAddr = res_int;
                            break;
                        case UINT_32:
                            res_li = std::stol((char*)text);
                            *(uint32_t*)module_array->at(i)->fwCfgList[j].targetAddr = (uint32_t)res_li;
                            break;
                        case VECT_INT32:
#ifdef _MSC_VER
                            token = strtok_s((char*)text, s, &next_token);
#else
                            token = strtok((char*)text, s);
#endif
                            m = 0;
                            while (token != nullptr && m< (module_array->at(i)->fwCfgList[j].max_len)) {
                                int vect_int = std::stoi(token);
                                if (m >= ((std::vector<int32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->size())
                                    ((std::vector<int32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->push_back(vect_int);
                                else
                                    ((std::vector<int32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->at(m) = vect_int;
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
                            while (token != nullptr && m < (module_array->at(i)->fwCfgList[j].max_len)) {
                                long int vect_lint = std::stol(token);
                                if (m >= ((std::vector<int32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->size())
                                    ((std::vector<uint32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->push_back((uint32_t)vect_lint);
                                else
                                    ((std::vector<uint32_t>*)(module_array->at(i)->fwCfgList[j].targetAddr))->at(m) = (uint32_t)vect_lint;
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

void fw_manager::read_xml_cfg(char *xmlFileName)
{
    LIBXML_TEST_VERSION

    exampleFunc(xmlFileName, &this->fw_list);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
}