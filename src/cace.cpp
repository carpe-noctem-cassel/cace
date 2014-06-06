#include <cace.h>

namespace cace
{

	Cace* Cace::get()
	{
		static Cace instance;
		return &instance;
	}

	Cace::Cace()
	{
		timeManager = new TimeManager(communication);
		variableStore = new CVariableStore(this);
	}

}
int main()
{
	return 1;
}

