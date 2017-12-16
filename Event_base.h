#ifndef __EVENT_BASE_H__
#define __EVENT_BASE_H__
#include "Util.h"

class EventBases : public noncopyable
{
	//	virtual EventBase* allocBase() = 0;	
};

class EventBase : public EventBases 
{
	public:
		EventBase();
		~EventBase();
		void loop();
		bool cancel();

		EventBase& exit();
		bool exited();
		void wakeup();
		
};





#endif
