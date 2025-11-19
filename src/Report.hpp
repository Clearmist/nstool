#pragma once
#include <string>
#include <vector>
#include <iosfwd>
#include "../deps/json/json.hpp"

class Report
{
public:
    // The text line type.
    enum class TextType
    {
        Basic,
        Extended,
        Layout,
        Keydata
    };

    // JSON entry.
    void set(const std::string& path, const std::string& value);
    void set(const std::string& path, const char* value);
    void set(const std::string& path, int64_t value);
    void set(const std::string& path, uint64_t value);
    void set(const std::string& path, double value);
    void set(const std::string& path, bool value);
    void set(const std::string& path, const nlohmann::json& value);

    // JSON array entry: append to an array at "path".
    void push(const std::string& path, const std::string& value);
    void push(const std::string& path, const char* value);
    void push(const std::string& path, int64_t value);
    void push(const std::string& path, uint64_t value);
    void push(const std::string& path, double value);
    void push(const std::string& path, bool value);
    void push(const std::string& path, const nlohmann::json& value);

    // TEXT entry.
    void text(const std::string& line, TextType type = TextType::Basic);

    // Output.
    void write_text(std::ostream& os) const;
    void write_json(std::ostream& os) const;
    void write(std::ostream& os) const;

    // Setting output options.
    void setShowBasicInfo(bool v) { showBasicInfo = v; }
    void setShowExtendedInfo(bool v) { showExtendedInfo = v; }
    void setShowLayout(bool v) { showLayout = v; }
    void setShowKeydata(bool v) { showKeydata = v; }
    void setShowMachineReadable(bool v) { showMachineReadable = v; }

    bool getShowBasicInfo() const { return showBasicInfo; }
    bool getShowExtendedInfo() const { return showExtendedInfo; }
    bool getShowLayout() const { return showLayout; }
    bool getShowKeydata() const { return showKeydata; }
    bool getShowMachineReadable() const { return showMachineReadable; }
private:
    // For JSON mode.
    nlohmann::json root_;

    // For plain text mode.
    struct TextEntry
    {
        std::string line;
        TextType type;
    };
    std::vector<TextEntry> text_lines_;

    bool showBasicInfo = true;
    bool showExtendedInfo = false;
    bool showLayout = false;
    bool showKeydata = false;
    bool showMachineReadable = false;
};

Report& get_report();
