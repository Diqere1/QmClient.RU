#include "qm_live_protocol.h"

const char *QmLiveDenyReasonString(EQmLiveDenyReason Reason)
{
	switch(Reason)
	{
	case EQmLiveDenyReason::UNSUPPORTED:
		return "UNSUPPORTED";
	case EQmLiveDenyReason::DISABLED:
		return "DISABLED";
	case EQmLiveDenyReason::FULL:
		return "FULL";
	case EQmLiveDenyReason::VERSION_MISMATCH:
		return "VERSION_MISMATCH";
	}

	return "UNSUPPORTED";
}

EQmLiveDenyReason QmLiveDenyReasonFromInt(int Reason)
{
	switch(static_cast<EQmLiveDenyReason>(Reason))
	{
	case EQmLiveDenyReason::UNSUPPORTED:
	case EQmLiveDenyReason::DISABLED:
	case EQmLiveDenyReason::FULL:
	case EQmLiveDenyReason::VERSION_MISMATCH:
		return static_cast<EQmLiveDenyReason>(Reason);
	}

	return EQmLiveDenyReason::UNSUPPORTED;
}
