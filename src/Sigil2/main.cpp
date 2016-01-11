#include "SglClo.hpp"

int main(int argc, char* argv[])
{
	sgl::Clo clo(argc, argv);
	clo.parse();
}
