#ifndef ENGINE_SHARED_QM_LIVE_PROTOCOL_H
#define ENGINE_SHARED_QM_LIVE_PROTOCOL_H

inline constexpr int QM_LIVE_OBSERVER_PROTOCOL_VERSION = 1;

enum class EQmLiveDenyReason
{
	UNSUPPORTED = 0,
	DISABLED,
	FULL,
	VERSION_MISMATCH,
};

const char *QmLiveDenyReasonString(EQmLiveDenyReason Reason);
EQmLiveDenyReason QmLiveDenyReasonFromInt(int Reason);

#endif // ENGINE_SHARED_QM_LIVE_PROTOCOL_H
