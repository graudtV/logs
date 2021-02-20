#ifndef LOGS_H_
#define LOGS_H_

#include <fstream>
#include <iostream>
#include <chrono>
#include <string_view>
#include <vector>
#include <numeric>
#include <iomanip>

namespace lgs {

namespace detail {

inline std::string_view gettimestr()
{
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	return std::ctime(&time);
}
inline std::string_view gettimestr_noendl()
{
	auto res = gettimestr();
	res.remove_suffix(res.size() - res.find_last_not_of('\n') - 1);
	return res;
}

/* refers to an existing output stream or creates and manages a new one */
class OstreamHandler {
protected:
	OstreamHandler(std::ostream& stream)
		{ set_ostream(stream); }
	OstreamHandler(const char *filename, std::ios_base::openmode mode)
		{ set_ostream(filename, mode); }
	~OstreamHandler()
	{
		if (m_allocated)
			delete m_stream;
	}
	OstreamHandler(const OstreamHandler& other) = delete;
	OstreamHandler& operator =(const OstreamHandler& other) = delete;
	OstreamHandler(OstreamHandler&& other) = delete;
	OstreamHandler& operator =(OstreamHandler&& other) = delete;

	void set_ostream(std::ostream& stream)
		{ m_stream = &stream; m_allocated = false; }
	void set_ostream(const char *filename, std::ios_base::openmode mode)
		{ m_stream = new std::ofstream(filename, mode); m_allocated = true; }

protected:
	std::ostream *m_stream;
private:
	bool m_allocated;
};

} // detail namespace end

/* The main class for logging */
class EnabledLogger final : public detail::OstreamHandler {
public:
	EnabledLogger() : EnabledLogger(std::cerr) {}
	EnabledLogger(std::ostream& stream) : OstreamHandler(stream) {}
	EnabledLogger(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out) :
		OstreamHandler(filename.c_str(), mode) {}
	EnabledLogger(const char *filename, std::ios_base::openmode mode = std::ios_base::out) :
		OstreamHandler(filename, mode) {}

	static constexpr bool is_enabled() { return true; }

	void set_ostream(std::ostream& stream)
		{ OstreamHandler::set_ostream(stream); }
	void set_ostream(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out)
		{ OstreamHandler::set_ostream(filename.c_str(), mode); }
	void set_ostream(const char *filename, std::ios_base::openmode mode = std::ios_base::out)
		{ OstreamHandler::set_ostream(filename, mode); }

	template <class T>
	EnabledLogger& operator <<(const T& value)
	{
		*m_stream << value;
		return *this;
	}

	/* EnabledLogger supports all std::ostream manipulators */
	EnabledLogger& operator <<(std::ostream& (*manip)(std::ostream& os))
		{ return operator << <>(manip); }

	/* EnabledLogger's own manipulators support */
	EnabledLogger& operator <<(EnabledLogger& (*manip)(EnabledLogger& logs))
		{ return manip(*this); }

public:
	int entries_counter = 0;

	static EnabledLogger& end_entry(EnabledLogger& logs) { return logs << std::endl << std::endl; }
	static EnabledLogger& new_entry(EnabledLogger& logs)
		{ return logs << "#" << logs.entries_counter++ << " : " << detail::gettimestr() << "> "; }
	static EnabledLogger& new_item(EnabledLogger& logs) { return logs << std::endl << '\t'; }
};

/* Has the same interface as EnabledLogger, but all methods do nothing */
class DisabledLogger final {
public:
	DisabledLogger() {}
	DisabledLogger(std::ostream& /* stream */) {}
	DisabledLogger(const std::string& /* filename */, std::ios_base::openmode /* mode */ = std::ios_base::out) {}
	DisabledLogger(const char * /* filename */, std::ios_base::openmode /* mode */ = std::ios_base::out) {}

	static constexpr bool is_enabled() { return false; }

	void set_ostream(std::ostream& /* stream */) {}
	void set_ostream(const std::string& /* filename */, std::ios_base::openmode /* mode */ = std::ios_base::out) {}
	void set_ostream(const char * /* filename */, std::ios_base::openmode /* mode */ = std::ios_base::out) {}

	template <class T>
	DisabledLogger& operator <<(const T& /* value */) { return *this; }

	DisabledLogger& operator <<(std::ostream& (* /* manip */)(std::ostream& os)) { return *this; }
	DisabledLogger& operator <<(EnabledLogger& (* /* manip */)(EnabledLogger& logs)) { return *this; }
	DisabledLogger& operator <<(DisabledLogger& (* /* manip */)(DisabledLogger& logs)) { return *this; }
};

/* Manipulator. Similar to new_entry, but also inserts a tag on the top */
class new_entry_tagged {
public:
	explicit new_entry_tagged(const char *descr) : m_descr(descr) {}
	friend EnabledLogger& operator <<(EnabledLogger& logs, const new_entry_tagged& t)
	{
		return logs << "#" << logs.entries_counter++ << " : " << detail::gettimestr_noendl()
			<< "\t ----------- [ "<< t.m_descr << " ] ----------- " << std::endl << "> ";
	}
private:
	const char *m_descr; // tag value
};

/*  Provides interface for creating item lists and printing them
 *  Item list is automatically formatted (padded with spaces and tabs) when
 * is written to logs to look more readable
 * 	ItemList can be printed like this: m_logs << list;
 * 	Note. new_entry and end_entry are NOT needed for printing ItemList,
 * because it creates new entry itself */
class ItemList {
public:
	ItemList(const std::string& list_name) : m_list_name(list_name) {}
	ItemList& add_item(const std::string& text) { return add_item(text, ""); }
	ItemList& add_item(const std::string& text, const std::string& tag) { m_items.push_back({text, tag}); return *this; }

	/* sets tag for all items with existing_item name */
	ItemList& set_tag(const std::string& existing_item, const std::string& tag)
	{
		for (Item& item : m_items)
			if (item.text == existing_item)
				item.tag = tag;
		return *this;
	}

	ItemList& set_header_tag(const std::string& header_tag) { m_header_tag = header_tag; return *this; }
private:
	struct Item {
		std::string text;
		std::string tag;
	};
	std::vector<Item> m_items;
	std::string m_header_tag;
	std::string m_list_name;

	friend EnabledLogger& operator <<(EnabledLogger& logs, const ItemList& lst)
	{
		if (!lst.m_header_tag.empty())
			logs << new_entry_tagged(lst.m_header_tag.c_str());
		else
			logs << EnabledLogger::new_entry;
		logs << lst.m_list_name << std::endl;
		size_t max_len = std::accumulate(lst.m_items.begin(), lst.m_items.end(), 0,
			[](size_t sz, const Item& item) { return std::max(sz, item.text.size()); } );
		logs << std::left;
		for (const Item& item : lst.m_items) {
			if (!item.tag.empty())
				logs << '\t' << std::setw(max_len) << item.text << "\t [ " << item.tag << " ]" << std::endl;
			else
				logs << '\t' << item.text << std::endl;
		}
		return logs << std::endl;
	}
};

/* Manipulating logs format. For DisabledLogger does nothing */
inline const auto new_entry = EnabledLogger::new_entry;
inline const auto end_entry = EnabledLogger::end_entry;
inline const auto new_item = EnabledLogger::new_item;
inline DisabledLogger no_logs;

} // lgs namespace end

#endif