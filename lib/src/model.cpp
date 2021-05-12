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

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.hpp"
#include "private.hpp"
#include "utils.hpp"
#include <efyj/efyj.hpp>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <expat.h>

namespace efyj {

struct Model_reader
{
    Model_reader(const context& ctx_,
                 const input_file& is_,
                 Model& dex_) noexcept
      : ctx(ctx_)
      , is(is_)
      , dex(dex_)
      , m_status(status::success)
    {}

    status read(int buffer_size)
    {
        XML_Parser parser = XML_ParserCreate(NULL);
        scope_exit parser_free([&parser]() { XML_ParserFree(parser); });
        parser_data data(ctx, parser, dex);
        XML_SetElementHandler(
          parser, Model_reader::start_element, Model_reader::end_element);
        XML_SetCharacterDataHandler(parser, Model_reader::character_data);
        XML_SetUserData(parser, reinterpret_cast<void*>(&data));

        int done;
        XML_Size line = 0;
        XML_Size column = 0;

        do {
            char* buffer = (char*)XML_GetBuffer(parser, buffer_size);
            if (buffer == nullptr) {
                error(ctx,
                      "DExi fail to allocate expat buffer (size: {})",
                      buffer_size);

                m_status = status::not_enough_memory;
                break;
            }

            size_t szlen =
              fread(buffer, 1, static_cast<size_t>(buffer_size), is.get());
            int len = static_cast<int>(szlen);
            done = len < buffer_size;

            if (XML_ParseBuffer(parser, len, done) == XML_STATUS_ERROR) {
                if (is_success(m_status))
                    m_status = status::dexi_parser_file_format_error;

                line = XML_GetCurrentLineNumber(parser);
                column = XML_GetCurrentColumnNumber(parser);
                break;
            }
        } while (!done);

        if (is_bad(m_status)) {
            error_at_line = static_cast<size_t>(line);
            error_at_column = static_cast<size_t>(column);
        }

        return m_status;
    }

private:
    const context& ctx;
    const input_file& is;
    Model& dex;

    status m_status{ status::success };
    size_t error_at_line;
    size_t error_at_column;

    enum class stack_identifier
    {
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
        NORMLOCWEIGHTS,
        HIGH,
        OPTDATATYPE,
        OPTLEVELS,
        LINKING
    };

    static std::optional<stack_identifier> str_to_stack_identifier(
      const char* name)
    {
        // TODO replace with a sorted C const char* array to perform O(log(n))
        // search without using unordered_map.

        static const std::
          unordered_map<const char*, stack_identifier, str_hash, str_compare>
            stack_identifier_map(
              { { "DEXi", stack_identifier::DEXi },
                { "VERSION", stack_identifier::TAG_VERSION },
                { "CREATED", stack_identifier::CREATED },
                { "LINE", stack_identifier::LINE },
                { "OPTION", stack_identifier::OPTION },
                { "SETTINGS", stack_identifier::SETTINGS },
                { "FONTSIZE", stack_identifier::FONTSIZE },
                { "REPORTS", stack_identifier::REPORTS },
                { "ATTRIBUTE", stack_identifier::ATTRIBUTE },
                { "NAME", stack_identifier::NAME },
                { "DESCRIPTION", stack_identifier::DESCRIPTION },
                { "SCALE", stack_identifier::SCALE },
                { "ORDER", stack_identifier::ORDER },
                { "SCALEVALUE", stack_identifier::SCALEVALUE },
                { "GROUP", stack_identifier::GROUP },
                { "FUNCTION", stack_identifier::FUNCTION },
                { "LOW", stack_identifier::LOW },
                { "ENTERED", stack_identifier::ENTERED },
                { "CONSIST", stack_identifier::CONSIST },
                { "ROUNDING", stack_identifier::ROUNDING },
                { "WEIGHTS", stack_identifier::WEIGHTS },
                { "LOCWEIGHTS", stack_identifier::LOCWEIGHTS },
                { "NORMLOCWEIGHTS", stack_identifier::NORMLOCWEIGHTS },
                { "HIGH", stack_identifier::HIGH },
                { "OPTDATATYPE", stack_identifier::OPTDATATYPE },
                { "OPTLEVELS", stack_identifier::OPTLEVELS },
                { "LINKING", stack_identifier::LINKING } });

        auto it = stack_identifier_map.find(name);

        return it == stack_identifier_map.end()
                 ? std::nullopt
                 : std::make_optional(it->second);
    }

    struct parser_data
    {
        parser_data(const context& ctx_, XML_Parser parser_, Model& data_)
          : ctx(ctx_)
          , parser(parser_)
          , model(data_)
          , m_status(status::success)
        {}

        const context& ctx;
        XML_Parser parser;
        Model& model;
        std::stack<stack_identifier> stack;
        std::stack<attribute*> attributes_stack;
        std::string char_data;
        status m_status;

        void stop_parser(const status t)
        {
            error(ctx, "DEXi failed to read file\n");
            XML_StopParser(parser, XML_FALSE);
            m_status = t;
        }

        bool is_parent(::std::initializer_list<stack_identifier> list)
        {
            if (!stack.empty()) {
                for (stack_identifier id : list)
                    if (id == stack.top())
                        return true;
            }

            return false;
        }
    };

    static void start_element(void* data,
                              const char* element,
                              const char** attribute) noexcept
    {
        (void)attribute;
        parser_data* pd = reinterpret_cast<parser_data*>(data);
        pd->char_data.clear();

        auto opt_id = str_to_stack_identifier(element);
        if (!opt_id) {
            pd->stop_parser(status::dexi_parser_element_unknown);
            return;
        }

        stack_identifier id = *opt_id;

        switch (id) {
        case stack_identifier::DEXi:
            if (!pd->stack.empty()) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }

            pd->stack.push(id);
            break;

        case stack_identifier::TAG_VERSION:
            if (!pd->is_parent({ stack_identifier::DEXi })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::CREATED:
            if (!pd->is_parent({ stack_identifier::DEXi })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::LINE:
            if (!pd->is_parent({ stack_identifier::DESCRIPTION })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::OPTION:
            if (!pd->is_parent(
                  { stack_identifier::DEXi, stack_identifier::ATTRIBUTE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::SETTINGS:
            if (!pd->is_parent({ stack_identifier::DEXi })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::FONTSIZE:
            if (!pd->is_parent({ stack_identifier::SETTINGS })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::REPORTS:
            if (!pd->is_parent({ stack_identifier::SETTINGS })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::OPTDATATYPE:
            if (!pd->is_parent({ stack_identifier::SETTINGS })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::OPTLEVELS:
            if (!pd->is_parent({ stack_identifier::SETTINGS })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::LINKING:
            if (!pd->is_parent({ stack_identifier::SETTINGS })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::ATTRIBUTE:
            if (!pd->is_parent(
                  { stack_identifier::DEXi, stack_identifier::ATTRIBUTE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            pd->model.attributes.emplace_back("unaffected attribute");

            if (!pd->attributes_stack.empty())
                pd->attributes_stack.top()->push_back(
                  pd->model.attributes.size() - 1);

            pd->attributes_stack.push(&pd->model.attributes.back());
            break;

        case stack_identifier::NAME:
            if (!pd->is_parent({ stack_identifier::DEXi,
                                 stack_identifier::ATTRIBUTE,
                                 stack_identifier::SCALEVALUE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }

            break;

        case stack_identifier::DESCRIPTION:
            if (!pd->is_parent({ stack_identifier::DEXi,
                                 stack_identifier::ATTRIBUTE,
                                 stack_identifier::SCALEVALUE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }

            pd->stack.push(id);
            break;

        case stack_identifier::SCALE:
            if (!pd->is_parent({ stack_identifier::ATTRIBUTE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::ORDER:
            if (!pd->is_parent({ stack_identifier::SCALE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::SCALEVALUE:
            if (!pd->is_parent({ stack_identifier::SCALE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            pd->model.attributes.back().scale.scale.emplace_back(
              "unaffected scalevalue");

            if (pd->model.attributes.size() > 127) {
                pd->stop_parser(status::dexi_parser_scale_too_big);
                break;
            }

            break;

        case stack_identifier::GROUP:
            if (!pd->is_parent({ stack_identifier::SCALEVALUE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;

        case stack_identifier::FUNCTION:
            if (!pd->is_parent({ stack_identifier::ATTRIBUTE })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            pd->stack.push(id);
            break;

        case stack_identifier::LOW:
        case stack_identifier::ENTERED:
        case stack_identifier::CONSIST:
        case stack_identifier::WEIGHTS:
        case stack_identifier::LOCWEIGHTS:
        case stack_identifier::NORMLOCWEIGHTS:
        case stack_identifier::HIGH:
        case stack_identifier::ROUNDING:
            if (!pd->is_parent({ stack_identifier::FUNCTION })) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }
            break;
        }
    }

    static void end_element(void* data, const char* element) noexcept
    {
        parser_data* pd = reinterpret_cast<parser_data*>(data);

        auto opt_id = str_to_stack_identifier(element);
        if (!opt_id) {
            pd->stop_parser(status::dexi_parser_element_unknown);
            return;
        }

        stack_identifier id = *opt_id;

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
#if _WIN32
                auto ret = sscanf_s(pd->char_data.c_str(), "%d", &att);
#else
                auto ret = sscanf(pd->char_data.c_str(), "%d", &att);
#endif

                if (ret != 1)
                    debug(pd->ctx,
                          "Option with unreadable string `{}'. Use `0' "
                          "instead\n",
                          pd->char_data);

                pd->model.attributes.back().options.emplace_back(att);
            } else {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }

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

        case stack_identifier::OPTDATATYPE:
            pd->model.optdatatype.assign(pd->char_data);
            pd->stack.pop();
            break;

        case stack_identifier::OPTLEVELS:
            pd->model.optlevels.assign(pd->char_data);
            pd->stack.pop();
            break;

        case stack_identifier::LINKING:
            pd->model.linking.assign(pd->char_data);
            pd->stack.pop();
            break;

        case stack_identifier::ATTRIBUTE:
            pd->stack.pop();

            if (pd->attributes_stack.top()->children.empty()) {
                auto scale_size =
                  pd->attributes_stack.top()->scale.scale.size();

                if (!is_numeric_castable<scale_id>(scale_size))
                    pd->stop_parser(status::dexi_parser_file_format_error);

                pd->model.basic_attribute_scale_size.emplace_back(
                  static_cast<scale_id>(scale_size));
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
            if (pd->stack.top() != stack_identifier::DESCRIPTION) {
                pd->stop_parser(status::dexi_parser_file_format_error);
                break;
            }

            pd->stack.pop();

            if (pd->stack.top() == stack_identifier::ATTRIBUTE)
                pd->model.attributes.back().description.assign(pd->char_data);
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
                    it = static_cast<int>(pd->model.group.size()) - 1;
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
        case stack_identifier::ROUNDING:
        case stack_identifier::HIGH:
            break;
        }
    }

    static void character_data(void* data, const XML_Char* s, int len) noexcept
    {
        parser_data* pd = reinterpret_cast<parser_data*>(data);

        try {
            pd->char_data.append(s, len);
        } catch (...) {
            error(pd->ctx,
                  "dexi: bad alloc ({} + {})\n",
                  pd->char_data.size(),
                  len);
            XML_StopParser(pd->parser, XML_FALSE);
        }
    }
};

struct to_xml
{
    to_xml(const std::string& str)
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

struct Model_writer
{
    Model_writer(const context& ctx_,
                 const output_file& os_,
                 const Model& Model_data_) noexcept
      : ctx(ctx_)
      , os(os_)
      , dex(Model_data_)
      , space(0)
    {}

    status write()
    {
        if (!os.is_open())
            return status::dexi_writer_error;

        os.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<DEXi>\n"
                 "  <VERSION>{}</VERSION>\n"
                 "  <CREATED>{}</CREATED>\n"
                 "  <NAME>{}</NAME>\n"
                 "  <DESCRIPTION>\n",
                 escape(dex.version),
                 escape(dex.created),
                 escape(dex.name));

        for (const auto& desc : dex.description) {
            if (desc.empty())
                os.print("    <LINE/>\n");
            else
                os.print("    <LINE>{}</LINE>\n", escape(desc));
        }

        os.print("  </DESCRIPTION>\n");
        space = 2;
        write_Model_option(dex.options);

        os.print("  <SETTINGS>\n");
        if (!dex.reports.empty())
            os.print("    <REPORTS>{}</REPORTS>\n", escape(dex.reports));
        else
            os.print("    <REPORTS>6</REPORTS>\n", escape(dex.reports));

        if (!dex.optdatatype.empty())
            os.print("    <OPTDATATYPE>{}</OPTDATATYPE>\n",
                     escape(dex.optdatatype));

        if (!dex.optlevels.empty())
            os.print("    <OPTLEVELS>{}</OPTLEVELS>\n", escape(dex.optlevels));

        if (!dex.linking.empty())
            os.print("    <LINKING>{}</LINKING>\n", escape(dex.linking));

        os.print("  </SETTINGS>\n", escape(dex.reports));

        if (!dex.attributes.empty())
            write_Model_attribute(0);

        os.print("</DEXi>\n");

        return status::success;
    }

private:
    const context& ctx;
    const output_file& os;
    const Model& dex;
    int space;

    std::string escape(const std::string& orig)
    {
        std::string ret;

        for (char ch : orig) {
            switch (ch) {
            case '&':
                ret += "&amp;";
                break;
            case '\'':
                ret += "&apos;";
                break;
            case '"':
                ret += "&quot;";
                break;
            case '<':
                ret += "&lt;";
                break;
            case '>':
                ret += "&gt;";
                break;
            default:
                ret += ch;
                break;
            }
        }

        return ret;
    }

    void make_space() const
    {
        os.print("{:{}}", "", space);
    }

    void make_space(int adding) const
    {
        os.print("{:{}}", "", space + adding);
    }

    void write_Model_option(const std::vector<std::string>& opts)
    {
        for (const auto& opt : opts) {
            make_space();
            os.print("<OPTION>{}</OPTION>\n", escape(opt));
        }
    }

    void write_Model_option(const std::vector<int>& opts)
    {
        for (const auto& opt : opts) {
            make_space();
            os.print("<OPTION>{}</OPTION>\n", opt);
        }
    }

    void write_Null_Model_option()
    {
        for (size_t i = 0, e = dex.options.size(); i != e; ++i) {
            make_space();
            os.print("<OPTION>0</OPTION>\n");
        }
    }

    void write_Model_attribute(size_t child)
    {
        assert(child <= dex.attributes.size());
        const attribute& att(dex.attributes[child]);

        make_space();
        os.print("<ATTRIBUTE>\n");

        space += 2;

        make_space();
        os.print("<NAME>{}</NAME>\n", escape(att.name));
        make_space();
        os.print("<DESCRIPTION>{}</DESCRIPTION>\n", escape(att.description));
        make_space();
        os.print("<SCALE>\n");

        space += 2;

        if (!att.scale.scale.empty() && !att.scale.order) {
            make_space();
            os.print("<ORDER>NONE</ORDER>\n");
        }

        for (const auto& sv : att.scale.scale) {
            make_space();
            os.print("<SCALEVALUE>\n");
            make_space(2);
            os.print("<NAME>{}</NAME>\n", escape(sv.name));

            if (!sv.description.empty()) {
                make_space(2);
                os.print("<DESCRIPTION>{}</DESCRIPTION>\n",
                         escape(sv.description));
            }

            if (sv.group >= 0) {
                make_space(2);
                os.print("<GROUP>{}</GROUP>\n", escape(dex.group[sv.group]));
            }

            make_space();
            os.print("</SCALEVALUE>\n");
        }

        space -= 2;
        make_space();
        os.print("</SCALE>\n");

        if (!att.functions.empty()) {
            make_space();
            os.print("<FUNCTION>\n");

            if (!att.functions.low.empty()) {
                make_space(2);
                os.print("<LOW>{}</LOW>\n", att.functions.low);
            }

            if (!att.functions.entered.empty()) {
                make_space(2);
                os.print("<ENTERED>{}</ENTERED>\n", att.functions.entered);
            }

            if (!att.functions.consist.empty()) {
                make_space(2);
                os.print("<CONSIST>{}</CONSIST>\n", att.functions.consist);
            }

            make_space();
            os.print("</FUNCTION>\n");
        }

        if (att.options.size() < dex.options.size())
            write_Null_Model_option();
        else
            write_Model_option(att.options);

        for (const auto& i : att.children)
            write_Model_attribute(i);

        space -= 2;
        make_space();
        os.print("</ATTRIBUTE>\n");
    }
};

static void
reorder_basic_attribute(const Model& model,
                        size_t att,
                        std::vector<size_t>& out)
{
    if (model.attributes[att].is_basic())
        out.push_back(att);
    else
        for (auto child : model.attributes[att].children)
            reorder_basic_attribute(model, child, out);
}

status
Model::read(const context& ctx, const input_file& is)
{
    Model_reader dr(ctx, is, *this);

#ifdef BUFSIZ
    return dr.read(static_cast<size_t>(BUFSIZ));
#else
    return dr.read(static_cast<size_t>(4096));
#endif
}

status
Model::write(const context& ctx, const output_file& os)
{
    Model_writer dw(ctx, os, *this);
    return dw.write();
}

void
Model::clear()
{
    std::string().swap(name);
    std::string().swap(version);
    std::string().swap(created);
    std::vector<std::string>().swap(description);
    std::vector<std::string>().swap(options);
    std::vector<scale_id>().swap(basic_attribute_scale_size);
    std::vector<std::string>().swap(group);
    std::deque<attribute>().swap(attributes);
}

void
Model::clear_options()
{
    options.clear();

    for (auto& att : attributes)
        att.options.clear();
}

} // namespace efyj
