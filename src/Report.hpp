#pragma once
#include "../deps/json/json.hpp"
#include <iosfwd>
#include <string>
#include <vector>

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

    // Set a JSON entry.
    template <typename T> void set(const std::string &path, const T &value)
    {
        // Let the JSON library convert the type.
        nlohmann::json j = value;

        // Delegate to a helper.
        add_json_internal(path, j);
    }

    // Add to a JSON array entry.
    template <typename T> void push(const std::string &path, T &&value)
    {
        nlohmann::json j = std::forward<T>(value);
        push_json_internal(path, j);
    }

    void merge(const std::string &path, const nlohmann::json &value);

    // TEXT entry.
    void text(const std::string &line, TextType type = TextType::Basic);

    // Output.
    void write_text(std::ostream &os) const;
    void write_json(std::ostream &os) const;
    void write(std::ostream &os) const;

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

    void add_json_internal(const std::string &path, const nlohmann::json &value);
    void push_json_internal(const std::string &path, const nlohmann::json &value);
};

Report &get_report();
