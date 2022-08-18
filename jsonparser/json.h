#ifndef JSON_H_INCLUDED
#define JSON_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <variant>

namespace json
{
    struct json_type;

    using null_type = std::nullptr_t;
    using bool_type = bool;
    using number_type = double;
    using string_type = std::string;
    using array_type = std::vector<json_type>;
    using object_type = std::map<string_type, json_type>;

    using variant_type = std::variant<null_type, bool_type, number_type, string_type, array_type, object_type>;
 
    struct json_type : variant_type 
    { 
        using variant_type::variant_type;
    };

    class json_formatter
    {
        unsigned tabsize;
        unsigned tabs;

        std::string indent() const { return tabsize ? std::string(tabsize * tabs, ' ') : ""; }

        std::string format(null_type) { return indent() + "null"; }

        std::string format(bool_type b) { return indent() + (b ? "true": "false"); }

        std::string format(number_type d)
        {
            char buf[200];
            snprintf(buf, sizeof buf, "%g", d); 
            return indent() + buf; 
        }

        std::string format(const string_type &s)
        { 
            std::string r;
            unsigned char c;

            for ( unsigned i = 0 ; i < s.length() ; i++ )
                switch ( c = s[i] )
                {
                    case '"':   r += "\\\"";    break;
                    case '\\':  r += "\\\\";    break;
                    case '/':   r += "\\/";     break;
                    case '\b':  r += "\\b";     break;
                    case '\f':  r += "\\f";     break;
                    case '\n':  r += "\\n";     break;
                    case '\r':  r += "\\r";     break;
                    case '\t':  r += "\\t";     break;
                    default:    r += c;         break;
                }

            return indent() + '"' + r + '"'; 
        }

        std::string format(const array_type &a)
        {
            if ( a.size() == 0 )
                return indent() + "[ ]";

            std::string s = tabsize ? indent() + "[\n" : "[ ";

            tabs++;
            for ( unsigned i = 0 ; i < a.size() ; i++ )
            {
                s += format(a[i]); 
                if ( i < a.size() - 1 )
                    s += ',';
                s += tabsize ? '\n' : ' ';  
            }
            tabs--;

            return s + indent() + ']';
        }

        std::string format(const object_type &o)
        {
            if ( o.size() == 0 )
                return indent() + "{ }";

            std::string s = tabsize ? indent() + "{\n" : "{ ";

            tabs++;
            unsigned index = 0;
            for ( const auto &member : o )
            {
                s += format(member.first) + ": ";

                std::string r = format(member.second);
                if ( index++ < o.size() - 1 )
                    r += ',';
                r += tabsize ? '\n' : ' ';

                if ( tabsize )
                {
                    std::string d = r.substr(indent().length());
                    
                    if ( (d[0] == '[' && d[2] != ']') || (d[0] == '{' && d[2] != '}') )
                        s += '\n' + r;
                    else
                        s += d;
                }
                else 
                    s += r;
            }
            tabs--;

            return s + indent() + '}';
        }

    public:

        json_formatter(unsigned tabsize = 0, unsigned tabs = 0) : tabsize(tabsize), tabs(tabs) { }

        std::string format(const variant_type &v) 
        { 
            return std::visit([this](const auto &x) { return format(x); }, v); 
        }
    };
         
} // namespace json

#endif
