#include <sys/resource.h>

int main()
{
	struct rlimit limit;
	limit.rlim_cur = 0;
	limit.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &limit);
	return 0;
}
