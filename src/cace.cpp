
#include <cace.h>

namespace cace {

Cace* Cace::get()
{
	static Cace instance;
	return &instance;
}

}
int main()
{
	return 1;
}

