#ifndef SERVICE_HPP
#define SERVICE_HPP

class Service
{
public:
	enum struct ServiceState
	{
		STARTING,
		RUNNING,
		ENDING,
		STOPPED
	};
	
protected:
	
	ServiceState state = ServiceState::STOPPED;
public:
	virtual void start() = 0;
	virtual void run() = 0;
	virtual void end() = 0;

	virtual ~Service() = default;

	ServiceState getState() const { return state; }
};
#endif // SERVICE_HPP