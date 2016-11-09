/* Copyright (C) 2015-2016 INRA
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

#ifndef ORG_VLEPROJECT_EFYJ_MODEL_HPP
#define ORG_VLEPROJECT_EFYJ_MODEL_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <efyj/details/private.hpp>
#include <efyj/details/utils.hpp>
#include <expat.h>
#include <limits>
#include <stack>
#include <string>
#include <string>
#include <unordered_map>
#include <vector>

namespace efyj {

using scale_id = int;

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, bool>::type
is_valid_scale_id(T n) noexcept
{
    return n <= 127;
}

template <typename T>
constexpr typename std::enable_if<!std::is_unsigned<T>::value, bool>::type
is_valid_scale_id(T n) noexcept
{
    return n >= 0 && n <= 127;
}

constexpr scale_id scale_id_unknown() noexcept
{
    return std::numeric_limits<scale_id>::max();
}

struct scalevalue {
    scalevalue(const std::string &name_)
        : name(name_)
        , group(-1)
    {
    }

    std::string name;
    std::string description;
    int group;
};

struct function {
    std::string low;
    std::string entered;
    std::string consist;

    bool empty() const noexcept
    {
        return low.empty() and entered.empty() and consist.empty();
    }
};

struct scales {
    scales()
        : order(true)
    {
    }

    bool order;
    std::vector<scalevalue> scale;

    scale_id find_scale_value(const std::string &name) const
    {
        for (std::size_t i = 0, e = scale.size(); i != e; ++i) {
            if (scale[i].name == name) {
                if (not is_valid_scale_id(i)) {
                    throw dexi_parser_error("bad scale definition");
                }
                return static_cast<int>(i);
            }
        }

        throw dexi_parser_error(stringf("scale `%s' not found", name.c_str()));
    }

    scale_id size() const
    {
        if (not is_valid_scale_id(scale.size()))
            throw dexi_parser_error("bad scale definition");

        return static_cast<int>(scale.size());
    }
};

struct attribute {
    attribute(const std::string &name_)
        : name(name_)
    {
    }

    std::size_t children_size() const noexcept { return children.size(); }

    scale_id scale_size() const noexcept { return scale.size(); }

    bool is_basic() const noexcept { return children.empty(); }

    bool is_aggregate() const noexcept { return !children.empty(); }

    void push_back(std::size_t child) { children.emplace_back(child); }

    std::string name;
    std::string description;
    scales scale;
    function functions;
    std::vector<int> options;
    std::vector<std::size_t> children;
};

struct Model {
    std::string name;
    std::string version;
    std::string created;
    std::string reports;
    std::vector<std::string> description;
    std::vector<std::string> options;
    std::vector<scale_id> basic_attribute_scale_size;
    std::vector<std::string> group;
    std::deque<attribute> attributes;

    void read(std::istream &is);

    void write(std::ostream &os);

    /** Release all dynamically allocated memory. */
    void clear();

    bool empty() const noexcept { return attributes.empty(); }

    int group_id(const std::string &name) const
    {
        auto it = std::find(group.cbegin(), group.cend(), name);

        if (it == group.cend())
            return -1;

        return it - group.cbegin();
    }

    void write_options(std::ostream &os) const;
    options_data write_options() const;
};

bool operator<(const Model &lhs, const Model &rhs);
bool operator==(const Model &lhs, const Model &rhs);
bool operator!=(const Model &lhs, const Model &rhs);

struct str_compare {
    inline bool operator()(const char *lhs, const char *rhs) const noexcept
    {
        return std::strcmp(lhs, rhs) == 0;
    }
};

struct str_hash {
    inline size_t operator()(const char *str) const noexcept
    {
        size_t hash = 0;
        int c;

        while ((c = *str++) != '\0')
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
    }
};

struct Model_reader {
    Model_reader(std::istream &is, Model &dex) noexcept : is(is), dex(dex) {}

    inline void read(std::size_t buffer_size)
    {
        XML_Parser parser = XML_ParserCreate(NULL);
        scope_exit parser_free([&parser]() { XML_ParserFree(parser); });
        parser_data data(parser, dex);
        XML_SetElementHandler(
            parser, Model_reader::start_element, Model_reader::end_element);
        XML_SetCharacterDataHandler(parser, Model_reader::character_data);
        XML_SetUserData(parser, reinterpret_cast<void *>(&data));

        while (is.good() and not is.eof()) {
            char *buffer = (char *)XML_GetBuffer(parser, buffer_size);

            if (not buffer)
                throw std::bad_alloc();

            is.read(buffer, buffer_size);

            if (not::XML_ParseBuffer(parser, is.gcount(), is.eof()))
                throw dexi_parser_error(data.error_message,
                                        XML_GetCurrentLineNumber(parser),
                                        XML_GetCurrentColumnNumber(parser),
                                        XML_GetErrorCode(parser));
        }
    }

private:
    std::istream &is;
    Model &dex;

    enum class stack_identifier {
        DEXi,
        TAG_VERSION,
        CREATED,
        LINE,
        OPTION,
        SETTINGS,
        FONTSIZE,
        REPORTS,
        ATTRIBUTE,
        NAME,
        DESCRIPTION,
        SCALE,
        ORDER,
        SCALEVALUE,
        GROUP,
        FUNCTION,
        LOW,
        ENTERED,
        CONSIST,
        ROUNDING,
        WEIGHTS,
        LOCWEIGHTS,
        NORMLOCWEIGHTS
    };

    inline static stack_identifier str_to_stack_identifier(const char *name)
    {
        static const std::unordered_map<const char *,
                                        stack_identifier,
                                        str_hash,
                                        str_compare>
            stack_identifier_map(
                {{"DEXi", stack_identifier::DEXi},
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
                 {"ROUNDING", stack_identifier::ROUNDING},
                 {"WEIGHTS", stack_identifier::WEIGHTS},
                 {"LOCWEIGHTS", stack_identifier::LOCWEIGHTS},
                 {"NORMLOCWEIGHTS", stack_identifier::NORMLOCWEIGHTS}});

        try {
            return stack_identifier_map.at(name);
        }
        catch (const std::exception & /*e*/) {
            throw dexi_parser_error(std::string("unknown element: ") + name);
        }
    }

    struct parser_data {
        inline parser_data(XML_Parser parser, Model &data)
            : parser(parser)
            , model(data)
        {
        }

        XML_Parser parser;
        std::string error_message;
        Model &model;
        std::stack<stack_identifier> stack;
        std::stack<attribute *> attributes_stack;
        std::string char_data;

        void is_parent(std::initializer_list<stack_identifier> list)
        {
            if (not stack.empty()) {
                for (stack_identifier id : list)
                    if (id == stack.top())
                        return;
            }

            throw dexi_parser_error("Bad parent");
        }
    };

    inline static void start_element(void *data,
                                     const char *element,
                                     const char **attribute) noexcept
    {
        (void)attribute;
        parser_data *pd = reinterpret_cast<parser_data *>(data);
        pd->char_data.clear();

        try {
            stack_identifier id = str_to_stack_identifier(element);

            switch (id) {
            case stack_identifier::DEXi:
                if (!pd->stack.empty())
                    throw dexi_parser_error("Bad parent");

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
                pd->is_parent(
                    {stack_identifier::DEXi, stack_identifier::ATTRIBUTE});
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
                pd->is_parent(
                    {stack_identifier::DEXi, stack_identifier::ATTRIBUTE});
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
                               stack_identifier::SCALEVALUE});
                break;

            case stack_identifier::DESCRIPTION:
                pd->is_parent({stack_identifier::DEXi,
                               stack_identifier::ATTRIBUTE,
                               stack_identifier::SCALEVALUE});
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
                pd->model.attributes.back().scale.scale.emplace_back(
                    "unaffected scalevalue");

                if (not is_valid_scale_id(pd->model.attributes.size()))
                    throw dexi_parser_error(
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
            case stack_identifier::ROUNDING:
                pd->is_parent({stack_identifier::FUNCTION});
                break;
            }
        }
        catch (const std::exception &e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    inline static void end_element(void *data, const char *element) noexcept
    {
        parser_data *pd = reinterpret_cast<parser_data *>(data);

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
                    }
                    catch (...) {
                        throw dexi_parser_error(
                            std::string("Can not convert option string ") +
                            pd->char_data + std::string(" in integer"));
                    }

                    pd->model.attributes.back().options.emplace_back(att);
                }
                else
                    throw dexi_parser_error("bad stack");

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
                    auto scale_size =
                        pd->attributes_stack.top()->scale.scale.size();
                    pd->model.basic_attribute_scale_size.emplace_back(
                        scale_size);
                }

                pd->attributes_stack.pop();
                break;

            case stack_identifier::NAME:
                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->model.attributes.back().name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::DEXi)
                    pd->model.name.assign(pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->model.attributes.back().scale.scale.back().name.assign(
                        pd->char_data);

                break;

            case stack_identifier::DESCRIPTION:
                if (pd->stack.top() != stack_identifier::DESCRIPTION)
                    throw dexi_parser_error("DESCRIPTION");

                pd->stack.pop();

                if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                    pd->model.attributes.back().description.assign(
                        pd->char_data);
                else if (pd->stack.top() == stack_identifier::SCALEVALUE)
                    pd->model.attributes.back()
                        .scale.scale.back()
                        .description.assign(pd->char_data);

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
                    pd->model.attributes.back().functions.entered =
                        pd->char_data;

                break;

            case stack_identifier::CONSIST:
                if (pd->stack.top() == stack_identifier::FUNCTION)
                    pd->model.attributes.back().functions.consist =
                        pd->char_data;

                break;
            case stack_identifier::WEIGHTS:
            case stack_identifier::LOCWEIGHTS:
            case stack_identifier::NORMLOCWEIGHTS:
            case stack_identifier::ROUNDING:
                break;
            }
        }
        catch (const std::exception &e) {
            pd->error_message = e.what();
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }

    inline static void
    character_data(void *data, const XML_Char *s, int len) noexcept
    {
        parser_data *pd = reinterpret_cast<parser_data *>(data);

        try {
            pd->char_data.append(s, len);
        }
        catch (const std::exception &e) {
            pd->error_message = "Bad alloc";
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }
};

struct to_xml {

    inline to_xml(const std::string &str)
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

inline std::ostream &operator<<(std::ostream &os, const to_xml &str)
{
    return os << str.m_str;
}

struct Model_writer {
    inline Model_writer(std::ostream &os, const Model &Model_data) noexcept
        : os(os),
          dex(Model_data),
          space(0)
    {
    }

    inline void write()
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
    const Model &dex;
    std::size_t space;

    inline std::string make_space() const { return std::string(space, ' '); }

    inline std::string make_space(std::size_t adding) const
    {
        return std::string(space + adding, ' ');
    }

    void write_Model_option(const std::vector<std::string> &opts)
    {
        for (const auto &opt : opts)
            os << make_space() << "<OPTION>" << opt << "</OPTION>\n";
    }

    void write_Model_option(const std::vector<int> &opts)
    {
        for (const auto &opt : opts)
            os << make_space() << "<OPTION>" << opt << "</OPTION>\n";
    }

    inline void write_Model_attribute(std::size_t child)
    {
        assert(child <= dex.attributes.size());
        const attribute &att(dex.attributes[child]);
        os << make_space() << "<ATTRIBUTE>\n";
        space += 2;
        os << make_space() << "<NAME>" << to_xml(att.name) << "</NAME>\n"
           << make_space() << "<DESCRIPTION>" << to_xml(att.description)
           << "</DESCRIPTION>\n"
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
                os << make_space(2) << "<GROUP>" << to_xml(dex.group[sv.group])
                   << "</GROUP>\n";

            os << make_space() << "</SCALEVALUE>\n";
        }

        space -= 2;
        os << make_space() << "</SCALE>\n";

        if (not att.functions.empty()) {
            os << make_space() << "<FUNCTION>\n";

            if (not att.functions.low.empty())
                os << make_space(2) << "<LOW>" << att.functions.low
                   << "</LOW>\n";

            if (not att.functions.entered.empty())
                os << make_space(2) << "<ENTERED>" << att.functions.entered
                   << "</ENTERED>\n";

            if (not att.functions.consist.empty())
                os << make_space(2) << "<CONSIST>" << att.functions.consist
                   << "</CONSIST>\n";

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

inline void reorder_basic_attribute(const Model &model,
                                    std::size_t att,
                                    std::vector<std::size_t> &out)
{
    if (model.attributes[att].is_basic())
        out.push_back(att);
    else
        for (auto child : model.attributes[att].children)
            reorder_basic_attribute(model, child, out);
}

inline void Model::write_options(std::ostream &os) const
{
    std::vector<std::size_t> ordered_att;
    reorder_basic_attribute(*this, 0, ordered_att);
    os << "simulation;place;department;year;";

    for (int child : ordered_att)
        os << attributes[child].name << ';';

    os << attributes[0].name << '\n';

    for (std::size_t opt = 0; opt != options.size(); ++opt) {
        os << options[opt] << "../;-;0;0;";

        for (int child : ordered_att)
            os << attributes[child]
                      .scale.scale[attributes[child].options[opt]]
                      .name
               << ';';

        os << attributes[0].scale.scale[attributes[0].options[opt]].name
           << '\n';
    }
}

inline options_data Model::write_options() const
{
    options_data ret;
    std::vector<std::size_t> ordered_att;
    reorder_basic_attribute(*this, 0, ordered_att);

    for (std::size_t opt = 0; opt != options.size(); ++opt) {
        ret.simulations.emplace_back(options[opt] + "../");
        ret.places.emplace_back("-");
        ret.departments.emplace_back(0);
        ret.years.emplace_back(0);
        ret.options.resize(ordered_att.size(), options.size());

        for (std::size_t c = 0, ec = ordered_att.size(); c != ec; ++c)
            ret.options(c, opt) = attributes[ordered_att[c]].options[opt];

        ret.observed.emplace_back(attributes[0].options[opt]);
    }

    return ret;
}

inline void Model::read(std::istream &is)
{
    Model_reader dr(is, *this);
    dr.read(4096u);
}

inline void Model::write(std::ostream &os)
{
    Model_writer dw(os, *this);
    dw.write();
}

inline void Model::clear()
{
    std::string().swap(name);
    std::string().swap(version);
    std::string().swap(created);
    std::string().swap(created);
    std::vector<std::string>().swap(description);
    std::vector<std::string>().swap(options);
    std::vector<scale_id>().swap(basic_attribute_scale_size);
    std::vector<std::string>().swap(group);
    std::deque<attribute>().swap(attributes);
}

inline bool operator==(const scalevalue &lhs, const scalevalue &rhs)
{
    return lhs.name == rhs.name && lhs.description == rhs.description &&
           lhs.group == rhs.group;
}

inline bool operator==(const scales &lhs, const scales &rhs)
{
    return lhs.order == rhs.order && lhs.scale == rhs.scale;
}

inline bool operator==(const function &lhs, const function &rhs)
{
    return lhs.low == rhs.low && lhs.entered == rhs.entered &&
           lhs.consist == rhs.consist;
}

inline bool operator==(const attribute &lhs, const attribute &rhs)
{
    return lhs.name == rhs.name && lhs.description == rhs.description &&
           lhs.scale == rhs.scale && lhs.functions == rhs.functions &&
           lhs.children == rhs.children;
}

inline bool operator<(const Model &lhs, const Model &rhs)
{
    return lhs.name < rhs.name;
}

inline bool operator==(const Model &lhs, const Model &rhs)
{
    return lhs.name == rhs.name && lhs.version == rhs.version &&
           lhs.created == rhs.created && lhs.description == rhs.description &&
           lhs.options == rhs.options && lhs.reports == rhs.reports &&
           lhs.basic_attribute_scale_size == rhs.basic_attribute_scale_size &&
           lhs.group == rhs.group && lhs.attributes == rhs.attributes;
}

inline bool operator!=(const Model &lhs, const Model &rhs)
{
    return not(lhs == rhs);
}

inline bool operator<(const attribute &lhs, const attribute &rhs)
{
    return lhs.name < rhs.name;
}

} // namespace efyj

#endif
