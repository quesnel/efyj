/* Copyright (C) 2014 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "model.hpp"
#include "print.hpp"
#include "utils.hpp"
#include <istream>
#include <ostream>
#include <functional>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <cstring>
#include <expat.h>

namespace {

struct str_compare
{
    bool operator()(const char* lhs, const char* rhs) const noexcept
    {
        return std::strcmp(lhs, rhs) == 0;
    }
};

struct str_hash
{
    size_t operator()(const char* str) const noexcept
    {
        size_t hash = 0;
        int c;

        while ((c = *str++) != '\0')
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
    }
};

struct dexi_reader
{
    dexi_reader(std::istream& is, efyj::dexi& dex) noexcept
    : is(is), dex(dex)
    {
    }

    void read(std::size_t buffer_size)
    {
        XML_Parser parser = XML_ParserCreate(NULL);
        efyj::scope_exit parser_free([&parser]() {XML_ParserFree(parser);});

        parser_data data(parser, dex);

        XML_SetElementHandler(parser, dexi_reader::start_element,
                              dexi_reader::end_element);
        XML_SetCharacterDataHandler(parser, dexi_reader::character_data);
        XML_SetUserData(parser, reinterpret_cast <void*>(&data));

        while (is.good() and not is.eof()) {
            char *buffer = (char*)XML_GetBuffer(parser, buffer_size);
            if (not buffer)
                throw std::bad_alloc();

            is.read(buffer, buffer_size);

            if (not ::XML_ParseBuffer(parser, is.gcount(), is.eof()))
                throw efyj::xml_parse_error(
                    data.error_message,
                    XML_GetCurrentLineNumber(parser),
                    XML_GetCurrentColumnNumber(parser),
                    XML_GetErrorCode(parser));
        }
    }

private:
    std::istream& is;
    efyj::dexi& dex;

    enum class stack_identifier {
        DEXi, LINE, OPTION, SETTINGS, FONTSIZE, REPORTS, ATTRIBUTE, NAME,
            DESCRIPTION, SCALE, ORDER, SCALEVALUE, GROUP, FUNCTION, LOW,
            ENTERED, CONSIST
            };

    static stack_identifier str_to_stack_identifier(const char* name)
    {
        static const std::unordered_map <
            const char*, stack_identifier, str_hash, str_compare>
            stack_identifier_map(
                {{"DEXi", stack_identifier::DEXi},
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
                    {"CONSIST", stack_identifier::CONSIST}});

        try {
            return stack_identifier_map.at(name);
        } catch (const std::exception& e) {
            throw std::invalid_argument(
                efyj::stringf("Unknown element: %s", name));
        }
    }

    struct parser_data
    {
        parser_data(XML_Parser parser, efyj::dexi& data)
            : parser(parser), dexi(data)
        {}

        XML_Parser parser;
        std::string error_message;
        efyj::dexi& dexi;
        std::stack <stack_identifier> stack;
        std::stack <efyj::attribute*> attributes_stack;
        std::string char_data;

        void is_parent(std::initializer_list <stack_identifier> list)
        {
            if (not stack.empty()) {
                for (stack_identifier id : list)
                    if (id == stack.top())
                        return;
            }

            throw std::invalid_argument("Bad parent");
        }
    };


    static void start_element(void *data, const char *element,
                              const char **attribute) noexcept
    {
        (void)attribute;
        parser_data *pd = reinterpret_cast <parser_data*>(data);
        pd->char_data.clear();

        try {
            stack_identifier id = str_to_stack_identifier(element);

            switch (id) {
            case stack_identifier::DEXi:
                if (!pd->stack.empty())
                    throw std::invalid_argument("Bad parent");
                pd->stack.push(id);
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
                pd->dexi.attributes.emplace_back("unaffected attribute");
                if (pd->attributes_stack.empty()) {
                    pd->dexi.child = &pd->dexi.attributes.back();
                } else {
                    pd->attributes_stack.top()->push_back(&pd->dexi.attributes.back());
                }
                pd->attributes_stack.push(&pd->dexi.attributes.back());
                break;
            case stack_identifier::NAME:
                pd->is_parent({stack_identifier::DEXi, stack_identifier::ATTRIBUTE, stack_identifier::SCALEVALUE});
                break;
            case stack_identifier::DESCRIPTION:
                pd->is_parent({stack_identifier::DEXi, stack_identifier::ATTRIBUTE, stack_identifier::SCALEVALUE});
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

                pd->dexi.attributes.back().scale.scale.emplace_back("unaffected scalevalue", pd->dexi.group.end());

                if (not efyj::is_valid_scale_id(pd->dexi.attributes.size()))
                    throw std::overflow_error(
                        efyj::stringf("Too many scale value (%" PRIuMAX ") "
                                      "for attribute `%s'",
                                      static_cast <std::uintmax_t>(pd->dexi.attributes.size()),
                                      pd->dexi.attributes.back().name.c_str()));
                break;
            case stack_identifier::GROUP:
                pd->is_parent({stack_identifier::SCALEVALUE});
                break;
            case stack_identifier::FUNCTION:
                pd->is_parent({stack_identifier::ATTRIBUTE});
                pd->stack.push(id);
                break;
            case stack_identifier::LOW:
                pd->is_parent({stack_identifier::FUNCTION});
                break;
            case stack_identifier::ENTERED:
                pd->is_parent({stack_identifier::FUNCTION});
                break;
            case stack_identifier::CONSIST:
                pd->is_parent({stack_identifier::FUNCTION});
                break;
            }
        } catch (const std::exception& e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    static void end_element(void *data, const char *element) noexcept
    {
        parser_data* pd = reinterpret_cast <parser_data*>(data);

        try {
            stack_identifier id = str_to_stack_identifier(element);

            switch (id) {
            case stack_identifier::DEXi:
                pd->stack.pop();
                break;
            case stack_identifier::LINE:
                break;
            case stack_identifier::OPTION:
                if (pd->stack.top() == stack_identifier::DEXi)
                    pd->dexi.options.emplace_back(pd->char_data);
                else if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->dexi.attributes.back().options.emplace_back(pd->char_data);
                else
                    throw std::invalid_argument("bad stack");
                break;
            case stack_identifier::SETTINGS:
                pd->stack.pop();
                break;
            case stack_identifier::FONTSIZE:
                pd->stack.pop();
                break;
            case stack_identifier::REPORTS:
                pd->stack.pop();
                break;
            case stack_identifier::ATTRIBUTE:
                pd->stack.pop();
                if (pd->attributes_stack.top()->children.empty()) {
                    pd->dexi.basic_scale_number++;

                    auto scale_size = pd->attributes_stack.top()->scale.scale.size();
                    pd->dexi.scalevalue_number += scale_size;
                    pd->dexi.problem_size *= scale_size;
                    pd->dexi.basic_attribute_scale_size.emplace_back(scale_size);
                }

                pd->attributes_stack.pop();
                break;
            case stack_identifier::NAME:
                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->dexi.attributes.back().name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::DEXi)
                    pd->dexi.name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->dexi.attributes.back().scale.scale.back().name.assign(pd->char_data);
                break;
            case stack_identifier::DESCRIPTION:
                if (pd->stack.top() != stack_identifier::DESCRIPTION)
                    throw std::invalid_argument("DESCRIPTION");

                pd->stack.pop();

                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->dexi.attributes.back().description.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::DEXi)
                    pd->dexi.description.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->dexi.attributes.back().scale.scale.back().description.assign(pd->char_data);
                break;
            case stack_identifier::SCALE:
                pd->stack.pop();
                pd->dexi.scale_number++;
                break;
            case stack_identifier::ORDER:
                if (pd->char_data == "NONE")
                    pd->dexi.attributes.back().scale.order = false;
                break;
            case stack_identifier::SCALEVALUE:
                pd->stack.pop();
                break;
            case stack_identifier::GROUP:
                if (pd->stack.top() == stack_identifier::SCALEVALUE) {
                    auto it = pd->dexi.group.find(pd->char_data);

                    if (it == pd->dexi.group.end())
                        it = pd->dexi.group.insert(pd->char_data).first;

                    pd->dexi.attributes.back().scale.scale.back().group = it;
                }
                break;
            case stack_identifier::FUNCTION:
                pd->stack.pop();
                break;
            case stack_identifier::LOW:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->dexi.attributes.back().functions.low = pd->char_data;
                break;
            case stack_identifier::ENTERED:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->dexi.attributes.back().functions.entered = pd->char_data;
                break;
            case stack_identifier::CONSIST:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->dexi.attributes.back().functions.consist = pd->char_data;
                break;
            }
        } catch (const std::exception& e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    static void character_data(void *data, const XML_Char *s, int len) noexcept
    {
        parser_data* pd = reinterpret_cast <parser_data*>(data);

        try {
            pd->char_data.append(s, len);
        } catch (const std::exception& e) {
            pd->error_message = "Bad alloc";
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }
};

struct dexi_writer
{
    dexi_writer(std::ostream& os, const efyj::dexi& dexi_data) noexcept
    : os(os), dex(dexi_data), space(0)
    {}

    void write()
    {
        write_dexi();
    }

private:
    std::ostream& os;
    const efyj::dexi& dex;
    std::size_t space;

    std::string make_space() const
    {
        return std::move(std::string(space, ' '));
    }

    std::string make_space(std::size_t adding) const
    {
        return std::move(std::string(space + adding, ' '));
    }

    void write_dexi_option(const std::vector <std::string>& opts)
    {
        for (const auto& opt : opts)
            os << make_space() << "<OPTION>" << opt << "</OPTION>\n";
    }

    void write_dexi_attribute(const efyj::attribute& att)
    {
        os << make_space() << "<ATTRIBUTE>\n";

        space += 2;
        os << make_space() << "<NAME>" << att.name << "</NAME>\n"
           << make_space() << "<DESCRIPTION>" << att.description << "</DESCRIPTION>\n"
           << make_space() << "<SCALE>\n";

        space += 2;
        if (not att.scale.scale.empty() and not att.scale.order)
            os << make_space() << "<ORDER>NONE</ORDER>\n";

        for (const auto& sv : att.scale.scale) {
            os << make_space() << "<SCALEVALUE>\n"
               << make_space(2) << "<NAME>" << sv.name << "</NAME>\n";

            if (not sv.description.empty())
                os << make_space(2) << "<DESCRIPTION>"
                   << sv.description << "</DESCRIPTION>\n";

            if (sv.group != dex.group.end())
                os << make_space(2) << "<GROUP>"
                   << *sv.group << "</GROUP>\n";

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
                   << att.functions.entered<< "</ENTERED>\n";

            if (not att.functions.consist.empty())
                os << make_space(2) << "<CONSIST>"
                   << att.functions.consist << "</CONSIST>\n";

            os << make_space() << "</FUNCTION>\n";
        }

        if (not att.options.empty())
            write_dexi_option(att.options);

        for (const auto& child : att.children)
            write_dexi_attribute(*child);
        space -= 2;

        os << make_space() << "</ATTRIBUTE>\n";
    }

    void write_dexi()
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<DEXi>\n"
           << "  <NAME>" << dex.name << "</NAME>\n";

        space = 2;
        write_dexi_option(dex.options);

        if (dex.child)
            write_dexi_attribute(*(dex.child));

        os << "</DEXi>\n";
    }
};

} // anonymous namespace

namespace efyj {

std::ostream& operator<<(std::ostream& os, const dexi& dexi_data)
{
    ::dexi_writer dw(os, dexi_data);

    dw.write();

    return os;
}

std::istream& operator>>(std::istream& is, dexi& dexi_data)
{
    ::dexi_reader dr(is, dexi_data);

    dr.read(4096u);

    return is;
}

}
