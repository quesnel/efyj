/* Copyright (C) 2015 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "model.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <algorithm>
#include <istream>
#include <ostream>
#include <functional>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <cassert>
#include <cstring>
#include <expat.h>

namespace {

struct str_compare {
    bool operator()(const char *lhs, const char *rhs) const noexcept
    {
        return std::strcmp(lhs, rhs) == 0;
    }
};

struct str_hash {
    size_t operator()(const char *str) const noexcept
    {
        size_t hash = 0;
        int c;

        while ((c = *str++) != '\0')
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
    }
};

struct Model_reader {
    Model_reader(std::istream &is, efyj::Model &dex) noexcept
        : is(is), dex(dex)
    {
    }

    void read(std::size_t buffer_size)
    {
        XML_Parser parser = XML_ParserCreate(NULL);
        efyj::scope_exit parser_free([&parser]() {XML_ParserFree(parser);});
        parser_data data(parser, dex);
        XML_SetElementHandler(parser, Model_reader::start_element,
                              Model_reader::end_element);
        XML_SetCharacterDataHandler(parser, Model_reader::character_data);
        XML_SetUserData(parser, reinterpret_cast <void *>(&data));

        while (is.good() and not is.eof()) {
            char *buffer = (char *)XML_GetBuffer(parser, buffer_size);

            if (not buffer)
                throw std::bad_alloc();

            is.read(buffer, buffer_size);

            if (not ::XML_ParseBuffer(parser, is.gcount(), is.eof()))
                throw efyj::xml_parser_error(
                    data.error_message,
                    XML_GetCurrentLineNumber(parser),
                    XML_GetCurrentColumnNumber(parser),
                    XML_GetErrorCode(parser));
        }
    }

private:
    std::istream &is;
    efyj::Model &dex;

    enum class stack_identifier {
        DEXi, TAG_VERSION, CREATED, LINE, OPTION, SETTINGS, FONTSIZE, REPORTS,
        ATTRIBUTE, NAME, DESCRIPTION, SCALE, ORDER, SCALEVALUE, GROUP,
        FUNCTION, LOW, ENTERED, CONSIST, WEIGHTS, LOCWEIGHTS, NORMLOCWEIGHTS
    };

    static stack_identifier str_to_stack_identifier(const char *name)
    {
        static const std::unordered_map <
        const char *, stack_identifier, str_hash, str_compare >
        stack_identifier_map( {
            {"DEXi", stack_identifier::DEXi},
            {"VERSION", stack_identifier::TAG_VERSION},
            {"CREATED", stack_identifier::CREATED},
            {"LINE", stack_identifier::LINE},
            {"OPTION", stack_identifier::OPTION},
            {"SETTINGS", stack_identifier::SETTINGS},
            {"FONTSIZE", stack_identifier::FONTSIZE},
            {"REPORTS", stack_identifier::REPORTS},
            {"ATTRIBUTE", stack_identifier::ATTRIBUTE},
            {"NAME", stack_identifier::NAME},
            {"DESCRIPTION", stack_identifier::DESCRIPTION},
            {"SCALE", stack_identifier::SCALE},
            {"ORDER", stack_identifier::ORDER},
            {"SCALEVALUE", stack_identifier::SCALEVALUE},
            {"GROUP", stack_identifier::GROUP},
            {"FUNCTION", stack_identifier::FUNCTION},
            {"LOW", stack_identifier::LOW},
            {"ENTERED", stack_identifier::ENTERED},
            {"CONSIST", stack_identifier::CONSIST},
            {"WEIGHTS", stack_identifier::WEIGHTS},
            {"LOCWEIGHTS", stack_identifier::LOCWEIGHTS},
            {"NORMLOCWEIGHTS", stack_identifier::NORMLOCWEIGHTS}
        });

        try {
            return stack_identifier_map.at(name);
        } catch (const std::exception & /*e*/) {
            throw efyj::xml_parser_error(
                std::string("unknown element: ") + name);
        }
    }

    struct parser_data {
        parser_data(XML_Parser parser, efyj::Model &data)
            : parser(parser), model(data)
        {}

        XML_Parser parser;
        std::string error_message;
        efyj::Model &model;
        std::stack <stack_identifier> stack;
        std::stack <efyj::attribute *> attributes_stack;
        std::string char_data;

        void is_parent(std::initializer_list <stack_identifier> list)
        {
            if (not stack.empty()) {
                for (stack_identifier id : list)
                    if (id == stack.top())
                        return;
            }

            throw efyj::xml_parser_error("Bad parent");
        }
    };


    static void start_element(void *data, const char *element,
                              const char **attribute) noexcept
    {
        (void)attribute;
        parser_data *pd = reinterpret_cast <parser_data *>(data);
        pd->char_data.clear();

        try {
            stack_identifier id = str_to_stack_identifier(element);

            switch (id) {
            case stack_identifier::DEXi:
                if (!pd->stack.empty())
                    throw efyj::xml_parser_error("Bad parent");

                pd->stack.push(id);
                break;

            case stack_identifier::TAG_VERSION:
                pd->is_parent({stack_identifier::DEXi});
                break;

            case stack_identifier::CREATED:
                pd->is_parent({stack_identifier::DEXi});
                break;

            case stack_identifier::LINE:
                pd->is_parent({stack_identifier::DESCRIPTION});
                break;

            case stack_identifier::OPTION:
                pd->is_parent({stack_identifier::DEXi, stack_identifier::ATTRIBUTE});
                break;

            case stack_identifier::SETTINGS:
                pd->is_parent({stack_identifier::DEXi});
                pd->stack.push(id);
                break;

            case stack_identifier::FONTSIZE:
                pd->is_parent({stack_identifier::SETTINGS});
                pd->stack.push(id);
                break;

            case stack_identifier::REPORTS:
                pd->is_parent({stack_identifier::SETTINGS});
                pd->stack.push(id);
                break;

            case stack_identifier::ATTRIBUTE:
                pd->is_parent({stack_identifier::DEXi, stack_identifier::ATTRIBUTE});
                pd->stack.push(id);
                pd->model.attributes.emplace_back("unaffected attribute");

                if (not pd->attributes_stack.empty())
                    pd->attributes_stack.top()->push_back(
                        pd->model.attributes.size() - 1);

                pd->attributes_stack.push(&pd->model.attributes.back());
                break;

            case stack_identifier::NAME:
                pd->is_parent({stack_identifier::DEXi,
                               stack_identifier::ATTRIBUTE,
                               stack_identifier::SCALEVALUE
                              });
                break;

            case stack_identifier::DESCRIPTION:
                pd->is_parent({stack_identifier::DEXi,
                               stack_identifier::ATTRIBUTE,
                               stack_identifier::SCALEVALUE
                              });
                pd->stack.push(id);
                break;

            case stack_identifier::SCALE:
                pd->is_parent({stack_identifier::ATTRIBUTE});
                pd->stack.push(id);
                break;

            case stack_identifier::ORDER:
                pd->is_parent({stack_identifier::SCALE});
                break;

            case stack_identifier::SCALEVALUE:
                pd->is_parent({stack_identifier::SCALE});
                pd->stack.push(id);
                pd->model.attributes.back().scale.scale.emplace_back("unaffected scalevalue");

                if (not efyj::is_valid_scale_id(pd->model.attributes.size()))
                    throw efyj::xml_parser_error(
                        std::string("Too many scale value for attribute: ") +
                        pd->model.attributes.back().name);

                break;

            case stack_identifier::GROUP:
                pd->is_parent({stack_identifier::SCALEVALUE});
                break;

            case stack_identifier::FUNCTION:
                pd->is_parent({stack_identifier::ATTRIBUTE});
                pd->stack.push(id);
                break;

            case stack_identifier::LOW:
            case stack_identifier::ENTERED:
            case stack_identifier::CONSIST:
            case stack_identifier::WEIGHTS:
            case stack_identifier::LOCWEIGHTS:
            case stack_identifier::NORMLOCWEIGHTS:
                pd->is_parent({stack_identifier::FUNCTION});
                break;
            }
        } catch (const std::exception &e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    static void end_element(void *data, const char *element) noexcept
    {
        parser_data *pd = reinterpret_cast <parser_data *>(data);

        try {
            stack_identifier id = str_to_stack_identifier(element);

            switch (id) {
            case stack_identifier::DEXi:
                pd->stack.pop();
                break;

            case stack_identifier::TAG_VERSION:
                pd->model.version.assign(pd->char_data);
                break;

            case stack_identifier::CREATED:
                pd->model.created.assign(pd->char_data);
                break;

            case stack_identifier::LINE:
                pd->model.description.emplace_back(pd->char_data);
                break;

            case stack_identifier::OPTION:
                if (pd->stack.top() == stack_identifier::DEXi)
                    pd->model.options.emplace_back(pd->char_data);
                else if (pd->stack.top() == stack_identifier::ATTRIBUTE) {
                    int att = 0;

                    try {
                        att = std::stoi(pd->char_data);
                    } catch (...) {
                        throw efyj::xml_parser_error(
                            std::string("Can not convert option string ") +
                            pd->char_data + std::string(" in integer"));
                    }

                    pd->model.attributes.back().options.emplace_back(att);
                } else
                    throw efyj::xml_parser_error("bad stack");

                break;

            case stack_identifier::SETTINGS:
                pd->stack.pop();
                break;

            case stack_identifier::FONTSIZE:
                pd->stack.pop();
                break;

            case stack_identifier::REPORTS:
                pd->model.reports.assign(pd->char_data);
                pd->stack.pop();
                break;

            case stack_identifier::ATTRIBUTE:
                pd->stack.pop();

                if (pd->attributes_stack.top()->children.empty()) {
                    auto scale_size = pd->attributes_stack.top()->scale.scale.size();
                    pd->model.basic_attribute_scale_size.emplace_back(scale_size);
                }

                pd->attributes_stack.pop();
                break;

            case stack_identifier::NAME:
                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->model.attributes.back().name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::DEXi)
                    pd->model.name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->model.attributes.back().scale.scale.back().name.assign(pd->char_data);

                break;

            case stack_identifier::DESCRIPTION:
                if (pd->stack.top() != stack_identifier::DESCRIPTION)
                    throw efyj::xml_parser_error("DESCRIPTION");

                pd->stack.pop();

                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->model.attributes.back().description.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->model.attributes.back().scale.scale.back().description.assign(
                        pd->char_data);

                break;

            case stack_identifier::SCALE:
                pd->stack.pop();
                break;

            case stack_identifier::ORDER:
                if (pd->char_data == "NONE")
                    pd->model.attributes.back().scale.order = false;

                break;

            case stack_identifier::SCALEVALUE:
                pd->stack.pop();
                break;

            case stack_identifier::GROUP:
                if (pd->stack.top() == stack_identifier::SCALEVALUE) {
                    int it = pd->model.group_id(pd->char_data);

                    if (it < 0) {
                        pd->model.group.emplace_back(pd->char_data);
                        it = pd->model.group.size() - 1;
                    }

                    pd->model.attributes.back().scale.scale.back().group = it;
                }

                break;

            case stack_identifier::FUNCTION:
                pd->stack.pop();
                break;

            case stack_identifier::LOW:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->model.attributes.back().functions.low = pd->char_data;

                break;

            case stack_identifier::ENTERED:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->model.attributes.back().functions.entered = pd->char_data;

                break;

            case stack_identifier::CONSIST:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->model.attributes.back().functions.consist = pd->char_data;

                break;
            case stack_identifier::WEIGHTS:
            case stack_identifier::LOCWEIGHTS:
            case stack_identifier::NORMLOCWEIGHTS:
                break;
            }
        } catch (const std::exception &e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    static void character_data(void *data, const XML_Char *s, int len) noexcept
    {
        parser_data *pd = reinterpret_cast <parser_data *>(data);

        try {
            pd->char_data.append(s, len);
        } catch (const std::exception &e) {
            pd->error_message = "Bad alloc";
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }
};

struct to_xml {

    to_xml(const std::string &str)
    {
        m_str.reserve(str.size() * 2);

        for (char ch : str) {
            switch (ch) {
            case '&':
                m_str += "&amp;";
                break;

            case '\'':
                m_str += "&apos;";
                break;

            case '"':
                m_str += "&quot;";
                break;

            case '<':
                m_str += "&lt;";
                break;

            case '>':
                m_str += "&gt;";
                break;

            default:
                m_str += ch;
                break;
            }
        }
    }

    std::string m_str;
};

std::ostream &operator<<(std::ostream &os, const to_xml &str)
{
    return os << str.m_str;
}

struct Model_writer {
    Model_writer(std::ostream &os, const efyj::Model &Model_data) noexcept
        : os(os), dex(Model_data), space(0)
    {}

    void write()
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<DEXi>\n"
           << "  <VERSION>" << to_xml(dex.version) << "</VERSION>\n"
           << "  <CREATED>" << to_xml(dex.created) << "</CREATED>\n"
           << "  <NAME>" << to_xml(dex.name) << "</NAME>\n"
           << "  <DESCRIPTION>\n";

        for (const auto &desc : dex.description) {
            if (desc.empty())
                os << "    <LINE/>\n";
            else
                os << "    <LINE>" << to_xml(desc) << "</LINE>\n";
        }

        os << "  </DESCRIPTION>\n";
        space = 2;
        write_Model_option(dex.options);

        if (!dex.reports.empty()) {
            os << "  <SETTINGS>\n"
               << "    <REPORTS>" << to_xml(dex.reports) << "</REPORTS>\n"
               << "  </SETTINGS>\n";
        }

        if (!dex.attributes.empty())
            write_Model_attribute(0);

        os << "</DEXi>\n";
    }

private:
    std::ostream &os;
    const efyj::Model &dex;
    std::size_t space;

    std::string make_space() const
    {
        return std::move(std::string(space, ' '));
    }

    std::string make_space(std::size_t adding) const
    {
        return std::move(std::string(space + adding, ' '));
    }

    template <typename T>
    void write_Model_option(const std::vector <T> &opts)
    {
        for (const auto &opt : opts)
            os << make_space() << "<OPTION>" << opt << "</OPTION>\n";
    }

    void write_Model_attribute(std::size_t child)
    {
        assert(child <= dex.attributes.size());
        const efyj::attribute &att(dex.attributes[child]);
        os << make_space() << "<ATTRIBUTE>\n";
        space += 2;
        os << make_space() << "<NAME>" << to_xml(att.name) << "</NAME>\n"
           << make_space() << "<DESCRIPTION>" << to_xml(att.description) <<
           "</DESCRIPTION>\n"
           << make_space() << "<SCALE>\n";
        space += 2;

        if (not att.scale.scale.empty() and not att.scale.order)
            os << make_space() << "<ORDER>NONE</ORDER>\n";

        for (const auto &sv : att.scale.scale) {
            os << make_space() << "<SCALEVALUE>\n"
               << make_space(2) << "<NAME>" << to_xml(sv.name) << "</NAME>\n";

            if (not sv.description.empty())
                os << make_space(2) << "<DESCRIPTION>"
                   << to_xml(sv.description) << "</DESCRIPTION>\n";

            if (sv.group >= 0)
                os << make_space(2) << "<GROUP>"
                   << to_xml(dex.group[sv.group]) << "</GROUP>\n";

            os << make_space() << "</SCALEVALUE>\n";
        }

        space -= 2;
        os << make_space() << "</SCALE>\n";

        if (not att.functions.empty()) {
            os << make_space() << "<FUNCTION>\n";

            if (not att.functions.low.empty())
                os << make_space(2) << "<LOW>"
                   << att.functions.low << "</LOW>\n";

            if (not att.functions.entered.empty())
                os << make_space(2) << "<ENTERED>"
                   << att.functions.entered << "</ENTERED>\n";

            if (not att.functions.consist.empty())
                os << make_space(2) << "<CONSIST>"
                   << att.functions.consist << "</CONSIST>\n";

            os << make_space() << "</FUNCTION>\n";
        }

        if (not att.options.empty())
            write_Model_option(att.options);

        for (const auto &child : att.children)
            write_Model_attribute(child);

        space -= 2;
        os << make_space() << "</ATTRIBUTE>\n";
    }
};

void reorder_basic_attribute(const efyj::Model &model, std::size_t att,
                             std::vector <std::size_t> &out)
{
    if (model.attributes[att].is_basic())
        out.push_back(att);
    else
        for (auto child : model.attributes[att].children)
            reorder_basic_attribute(model, child, out);
}

} // anonymous namespace

namespace efyj {

void Model::write_options(std::ostream &os) const
{
    std::vector <std::size_t> ordered_att;
    ::reorder_basic_attribute(*this, 0, ordered_att);
    os << "simulation;place;department;year;";

    for (int child : ordered_att)
        os << attributes[child].name << ';';

    os << attributes[0].name << '\n';

    for (std::size_t opt = 0; opt != options.size(); ++opt) {
        os << options[opt] << "../;-;0;0;";

        for (int child : ordered_att)
            os << attributes[child].scale.scale[attributes[child].options[opt]].name << ';';

        os << attributes[0].scale.scale[attributes[0].options[opt]].name << '\n';
    }
}


bool operator<(const attribute &lhs, const attribute &rhs)
{
    return lhs.name < rhs.name;
}


bool operator==(const scalevalue &lhs, const scalevalue &rhs)
{
    return lhs.name == rhs.name &&
           lhs.description == rhs.description &&
           lhs.group == rhs.group;
}


bool operator==(const scales &lhs, const scales &rhs)
{
    return lhs.order == rhs.order &&
           lhs.scale == rhs.scale;
}


bool operator==(const function &lhs, const function &rhs)
{
    return lhs.low == rhs.low &&
           lhs.entered == rhs.entered &&
           lhs.consist == rhs.consist;
}


bool operator==(const attribute &lhs, const attribute &rhs)
{
    return lhs.name == rhs.name &&
           lhs.description == rhs.description &&
           lhs.scale == rhs.scale &&
           lhs.functions == rhs.functions &&
           lhs.children == rhs.children;
}


bool operator<(const Model &lhs, const Model &rhs)
{
    return lhs.name < rhs.name;
}


bool operator==(const Model &lhs, const Model &rhs)
{
    return lhs.name == rhs.name &&
           lhs.version == rhs.version &&
           lhs.created == rhs.created &&
           lhs.description == rhs.description &&
           lhs.options == rhs.options &&
           lhs.reports == rhs.reports &&
           lhs.basic_attribute_scale_size == rhs.basic_attribute_scale_size &&
           lhs.group == rhs.group &&
           lhs.attributes == rhs.attributes;
}

bool operator!=(const Model &lhs, const Model &rhs)
{
    return not (lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const Model &Model_data)
{
    ::Model_writer dw(os, Model_data);
    dw.write();
    return os;
}

std::istream &operator>>(std::istream &is, Model &Model_data)
{
    ::Model_reader dr(is, Model_data);
    dr.read(4096u);
    return is;
}

}
