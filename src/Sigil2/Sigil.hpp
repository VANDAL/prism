#ifndef SGL_INSTANCE_H
#define SGL_INSTANCE_H

/* Sigil global instance */

#include "EventManager.hpp"
#include "SynchroTraceGen/EventHandlers.hpp"

class Sigil
{
	public:
		static Sigil& getInstance()
		{
			return sigil;
		}

	private:
		Sigil() {} 
		Sigil(Sigil const&) = delete;
		void operator=(Sigil const&) = delete;

	public:
		sgl::EventManager mgr;
	private:
		static Sigil sigil;
};

#endif
