[ ] make sure no path separator is passed when removing file ending
[ ] use realloc instead of memcpy and free
[x] move value size from function call to generic container struct
[ ] extract exposed functions that can be used by the user
[ ] define private function prototypes somewhere
[ ] hide ubuntu dependencies behind interface
[ ] find include dependencies and put in graph
[ ] make graph optional
[ ] make cache optional
[ ] recompile if file changed due to include dependency
[ ] use cache to populate graph
[ ] split generating dependency graph from indexing project
[ ] detect when .c file was included and disregard when compiling
[ ] detect if .h file is header only
[ ] use hash maps where appropriate
[x] remove duplication in growable arrays
[ ] implement generic hash map
[x] implement generic hash set for faster access
[x] implement generic array to reduce code duplication

