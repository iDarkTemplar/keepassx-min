#include <sys/prctl.h>

int main()
{
	prctl(PR_SET_DUMPABLE, 0);
	return 0;
}
