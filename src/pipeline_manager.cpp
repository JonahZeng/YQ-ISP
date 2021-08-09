#include "pipeline_manager.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

pipeline_manager::pipeline_manager()
{
    //需要构造一个statistic_info_t
    //
    stat_addr = new statistic_info_t;
    if (stat_addr == nullptr)
    {
        fprintf(stderr, "[waring] alloc memory for statistc_info fail\n");
    }
}

pipeline_manager::~pipeline_manager()
{
    for (vector<hw_base*>::iterator it = modules_list.begin(); it != modules_list.end(); it++)
    {
        delete (*it);
    }
    //需要释放一个statistic_info_t
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

static void exampleFunc(const char *filename, vector<hw_base*>* module_array) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;

    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
        return;
    }
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    if (ctxt->valid == 0)
    {
        fprintf(stderr, "Failed to validate %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    if (rootNode == NULL)
    {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    fprintf(stdout, "root node name: %s, type:%d\n", rootNode->name, rootNode->type); //pipeline_config
    xmlNodePtr component = rootNode->children;
    while (component != NULL)
    {
        fprintf(stdout, "component name %s, type:%d\n", component->name, component->type); //component

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
        while (detail_node != NULL) {
            if (xmlChildElementCount(detail_node) == 0) { //leaf node
                xmlChar* text = xmlNodeGetContent(detail_node);
                fprintf(stdout, "tag name:%s text:%s\n", detail_node->name, text);
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

}