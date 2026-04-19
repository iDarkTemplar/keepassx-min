#include <new>

int main()
{
	void *ptr = nullptr;
	std::size_t size = 1;
	::operator delete(ptr, size);
	return 0;
}
