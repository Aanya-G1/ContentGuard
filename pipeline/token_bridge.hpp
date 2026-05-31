#pragma once

#include <vector>
#include <string>

std::vector<std::string> getTokensBydocId(int docId);

void registerTokenBridge(class PreprocessorDB* ppdb);
