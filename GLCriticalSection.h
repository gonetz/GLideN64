#ifndef GLCRITICALSECTION_H
#define GLCRITICALSECTION_H

#include <mutex> // std::mutex

class GLCriticalSection
{
public:
	void lock()
	{
		m_mtx.lock();
		m_locked = true;
	}

	void unlock()
	{
		m_locked = false;
		m_mtx.unlock();
	}

	bool isLocked() const {return m_locked;}

protected:
	class Lock {
	public:
		Lock(GLCriticalSection * _pCS) : m_pCS(_pCS) {m_pCS->lock();}
		~Lock() {m_pCS->unlock();}
	private:
		GLCriticalSection * m_pCS;
	};

private:
	std::mutex m_mtx;
	bool m_locked;
};

#endif // GLCRITICALSECTION_H
