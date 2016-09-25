#ifndef PERFORMANCE_H
#define PERFORMANCE_H
#include <ctime>
#include "Types.h"

class Performance
{
public:
	Performance();
	void reset();
	f32 getFps() const;
	f32 getVIs() const;
	f32 getPercent() const;
	void increaseVICount();
	void increaseFramesCount();

private:
	u32 m_vi;
	u32 m_frames;
	f32 m_fps;
	f32 m_vis;
	clock_t m_startTime;
};

extern Performance perf;
#endif // PERFORMANCE_H
