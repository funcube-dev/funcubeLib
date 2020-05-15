#include "OverlappedFft.h"

COverlappedFft::COverlappedFft(void) : 
	m_current(NULL),
	m_previous(NULL),
	m_overlapSize(0)
{
}

COverlappedFft::~COverlappedFft(void)
{
}

BOOL COverlappedFft::Initialise(UINT numBins, UINT samplesPerBuffer)
{
	if(NULL!=m_current || NULL!=m_previous)
	{
		return FALSE;
	}

	// check bin count is power of 2
	if(numBins==0 || (numBins & (numBins-1)) != 0)
	{
		return FALSE;
	}

	// check samplesPerBuffer is multiple of num bins
	if(numBins%samplesPerBuffer != 0  || samplesPerBuffer > numBins)
	{
		return FALSE;
	}

	// check samplesPerBuffer is also a multiple of half num bins, so we
	// don't have to mess around copying part of a buffer between fft'ers
	if((numBins/2)%samplesPerBuffer != 0)
	{
		return FALSE;
	}

	m_overlapSize = numBins/2;
	
	MutexGuard guard(m_lock);

	m_current = new CFft();
	if(!m_current->Initialise(numBins, samplesPerBuffer))
	{
		Shutdown();
		return FALSE;
	}
	
	m_previous = new CFft();
	if(!m_previous->Initialise(numBins, samplesPerBuffer))
	{
		Shutdown();
		return FALSE;
	}

	// half fill the first buffer so we can now just
	// add samples and the swapping of buffers will 
	// look after itself as each buffer fills
	m_current->ZeroFill(m_overlapSize);

	return TRUE;
}

void COverlappedFft::Shutdown()
{
	MutexGuard guard(m_lock);

	if(m_previous!=NULL)
	{
		m_previous->Shutdown();
		delete m_previous;
		m_previous = NULL;
	}

	if(m_current!=NULL)
	{
		m_current->Shutdown();
		delete m_current;
		m_current = NULL;
	}
}

void COverlappedFft::Swap()
{
	MutexGuard guard(m_lock);

	CFft* temp = m_current;
	m_current = m_previous;
	m_previous = temp;
}

BOOL COverlappedFft::Add(const COMPLEXSTRUCT* samples, UINT sampleCount, BOOL& resultUpdated)
{	
	if(NULL==m_current || NULL==m_previous)
	{
		return FALSE;
	}

	BOOL success = TRUE;
		
	MutexGuard guard(m_lock);
	// add samples to current buffer, swap if results update
	if(!m_current->Add(samples, sampleCount, resultUpdated))
	{
		success = FALSE;
	}
	// add samples to other buffer, shouldn't update results
	BOOL updatedOutOfSync = FALSE;
	if(!m_previous->Add(samples, sampleCount, updatedOutOfSync))
	{
		success = FALSE;
	}

	if(updatedOutOfSync)
	{
		success = FALSE;
	}

	if(success && resultUpdated)
	{
		Swap();
	}
	
	return success;
}

BOOL COverlappedFft::IsOutputValid()
{
	MutexGuard guard(m_lock);

	if(NULL == m_previous || NULL == m_current)
	{
		return FALSE;
	}

	return m_previous->IsOutputValid() && m_current->IsOutputValid();

}

BOOL COverlappedFft::CopyResult(COMPLEXSTRUCT* samples, UINT sampleCount)
{
	MutexGuard guard(m_lock);

	if(!this->IsOutputValid())
	{
		return FALSE;
	}

	BOOL success = TRUE;
	// copy current result
	if(!m_current->CopyResult(samples, sampleCount))
	{
		success = FALSE;
	}
	
	// add in previous result
	if(!m_previous->SumResult(samples, sampleCount))
	{
		success = FALSE;
	}

	return success;
}

BOOL COverlappedFft::SumResult(COMPLEXSTRUCT* samples, UINT sampleCount)
{
	MutexGuard guard(m_lock);

	if(!this->IsOutputValid())
	{
		return FALSE;
	}

	BOOL success = TRUE;
	// add in current result
	if(!m_current->SumResult(samples, sampleCount))
	{
		success = FALSE;
	}
	
	// add in previous result
	if(!m_previous->SumResult(samples, sampleCount))
	{
		success = FALSE;
	}

	return success;
}

