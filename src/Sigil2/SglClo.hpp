namespace sgl
{
	class Clo
	{
		public:
			Clo(int argc, char* argv[]) 
				: argc(argc), argv(argv) {}

			void parse();
			
		private:
			int argc;
			char** argv;
	};
};
