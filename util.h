#pragma once
#include <string>
#include "pdfile.h"

unsigned long long getFileSize(const char* filename);
bool findNamespaceDetails(const std::string& nsFileName, const std::string& collname, NamespaceDetails& details);