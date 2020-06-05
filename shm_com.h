#define Number_Of_Elements 5
struct shared_use_st {
	enum {UNSORTED, SORTED} state[5];
	int B[Number_Of_Elements];
};

