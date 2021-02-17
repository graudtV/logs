#include "logs.h"
#include <iostream>
#include <chrono>
#include <sstream>

int main()
{
	/* Printing logs to an existing stream */
	lgs::EnabledLogger logs(std::cerr);

	/* Printing logs to a file: */
	// lgs::EnabledLogger logs("logs.txt");

	/* simple logs */
	logs << lgs::new_entry << "application started" << lgs::end_entry;

	/* items lists */
	logs << lgs::new_entry << "available options"
		<< lgs::new_item << "do nothing"
		<< lgs::new_item << "do almostly nothing"
		<< lgs::new_item << "do absolutely nothing" << lgs::end_entry;

	/*  More complex item lists with tags support.
	 *  On output items will be automatically padded with spaces and tabs
	 * to be more readable */
	lgs::ItemList lst("trying to find suitable GPU");
	lst
		.add_item("Intel GPU IrisTM", "zaebis")
		.add_item("AMD-vidiuha v15.2.1")
		.add_item("Virtual GPU Titan228")
		.add_item("Noname device from radiorinok")
		.set_tag("Virtual GPU Titan228", "SELECTED") // adding tag to an existing item
		.set_header_tag("OK"); // will be printed on the top, but can be set at any moment of item list creation
	logs << lst;

	/* printing log with header tag */
	logs << lgs::new_entry_tagged("FATAL ERROR") << "AHAHAHA SLOMALOS" << lgs::end_entry;
	return 0;
}