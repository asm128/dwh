#include "dwh_table.h"

int main() { 
	::dwh::STable		table;
	::dwh::tableOpen	(table, "Users");
	::dwh::tableClose	(table, "Users");
	return 0; 
}