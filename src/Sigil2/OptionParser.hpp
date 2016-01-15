#ifndef SGL_OPTIONPARSER_H
#define SGL_OPTIONPARSER_H

namespace sgl
{
class OptionParser
{
public:
	OptionParser(int argc, char* argv[]) 
		: argc(argc), argv(argv) {}

	void parse();
	
private:
	int argc;
	char** argv;
};
}; //end namespace sgl

#endif
