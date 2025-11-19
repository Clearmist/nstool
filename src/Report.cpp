#include "Report.hpp"

#include <sstream>
#include <ostream>

// -------------------------------
// Helpers
// -------------------------------

// Creates nested JSON objects for the given path.
static nlohmann::json& ensure_path(nlohmann::json& root, const std::string& path)
{
    std::stringstream ss(path);
    std::string segment;
    nlohmann::json* current = &root;

    while (std::getline(ss, segment, '.')) {
        if (segment.empty()) {
            continue;
        }

        // Automatically creates objects.
        current = &((*current)[segment]);
    }

    return *current;
}

// Creates (or converts) the node at path into an array.
static nlohmann::json& ensure_array_path(nlohmann::json& root, const std::string& path)
{
    nlohmann::json& node = ensure_path(root, path);

    if (node.is_null()) {
        // Nothing there yet: make it an empty array.
        node = nlohmann::json::array();
    }
    else if (!node.is_array()) {
        // Something is there but it's not an array: wrap it into an array.
        nlohmann::json old = node;
        node = nlohmann::json::array();
        node.push_back(std::move(old));
    }

    return node;
}

// -------------------------------
// JSON "add" functions
// -------------------------------

void Report::add(const std::string& path, const std::string& value)
{
    ensure_path(root_, path) = value;
}

void Report::add(const std::string& path, const char* value)
{
    ensure_path(root_, path) = value ? value : "";
}

void Report::add(const std::string& path, int64_t value)
{
    ensure_path(root_, path) = value;
}

void Report::add(const std::string& path, uint64_t value)
{
    ensure_path(root_, path) = value;
}

void Report::add(const std::string& path, double value)
{
    ensure_path(root_, path) = value;
}

void Report::add(const std::string& path, bool value)
{
    ensure_path(root_, path) = value;
}

// -------------------------------
// JSON "push" functions (arrays)
// -------------------------------

void Report::push(const std::string& path, const std::string& value)
{
    ensure_array_path(root_, path).push_back(value);
}

void Report::push(const std::string& path, const char* value)
{
    ensure_array_path(root_, path).push_back(value ? value : "");
}

void Report::push(const std::string& path, int64_t value)
{
    ensure_array_path(root_, path).push_back(value);
}

void Report::push(const std::string& path, uint64_t value)
{
    ensure_array_path(root_, path).push_back(value);
}

void Report::push(const std::string& path, double value)
{
    ensure_array_path(root_, path).push_back(value);
}

void Report::push(const std::string& path, bool value)
{
    ensure_array_path(root_, path).push_back(value);
}

void Report::push(const std::string& path, const nlohmann::json& value)
{
    ensure_array_path(root_, path).push_back(value);
}

// -------------------------------
// TEXT "add"
// -------------------------------

void Report::text(const std::string& line, TextType type)
{
    TextEntry entry;
    entry.line = line;
    entry.type = type;
    text_lines_.push_back(std::move(entry));
}

// -------------------------------
// Output
// -------------------------------

void Report::write_text(std::ostream& os) const
{
    for (const auto& entry : text_lines_) {
        switch (entry.type) {
        case TextType::Basic:
            if (showBasicInfo) {
                os << entry.line << "\n";
            }
            break;
        case TextType::Extended:
            if (showExtendedInfo) {
                os << entry.line << "\n";
            }
            break;
        case TextType::Layout:
            if (showLayout) {
                os << entry.line << "\n";
            }
            break;
        case TextType::Keydata:
            if (showKeydata) {
                os << entry.line << "\n";
            }
            break;
        }
    }
}

void Report::write_json(std::ostream& os) const
{
    // Dumping root_ will print "null" if it is empty.
    os << root_.dump(2) << "\n";
}

void Report::write(std::ostream& os) const
{
    if (showMachineReadable) {
        write_json(os);
    } else {
        write_text(os);
    }
}

// -------------------------------
// Singleton
// -------------------------------

Report& get_report()
{
    static Report instance;

    return instance;
}
