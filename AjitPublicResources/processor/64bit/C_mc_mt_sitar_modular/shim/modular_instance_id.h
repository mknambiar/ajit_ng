#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int modular_extract_first_index(const char* src, int* first);
int modular_extract_two_indices(const char* src, int* first, int* second);

#ifdef __cplusplus
}
#endif
