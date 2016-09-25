#include "VI.h"
#include "Performance.h"

Performance perf;

Performance::Performance()
	: m_vi(0)
	, m_frames(0)
	, m_fps(0)
	, m_vis(0)
	, m_startTime(0) {
}

void Performance::reset()
{
	m_startTime = std::clock();
}

f32 Performance::getFps() const
{
	return m_fps;
}

f32 Performance::getVIs() const
{
	return m_vis;
}

f32 Performance::getPercent() const
{
	const f32 scale = VI.PAL ? 0.5f : 0.6f;
	return m_vis / scale;
}

void Performance::increaseVICount()
{
	m_vi++;
	const clock_t curTime = std::clock();
	const float elapsed = float( curTime - m_startTime ) /  CLOCKS_PER_SEC;
	if (elapsed < 0.5)
		return;
	m_vis = m_vi / elapsed;
	m_fps = m_frames / elapsed;
	m_vi = 0;
	m_frames = 0;
	m_startTime = curTime;
}

void Performance::increaseFramesCount()
{
	m_frames++;
}
