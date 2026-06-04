/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_GPU_UPLOAD_LIMITER_H
#define ENGINE_CLIENT_GPU_UPLOAD_LIMITER_H

/**
 * Global GPU texture upload rate limiter.
 *
 * This class provides frame-based throttling for GPU texture uploads
 * to prevent frame stuttering caused by batch texture uploads.
 *
 * Usage:
 * - Call OnFrameStart() at the beginning of each frame
 * - Call CanUpload() before each texture upload
 * - Call OnUploaded() after each successful texture upload
 *
 * Thread Safety:
 * - This class is designed for single-threaded use on the main/render thread
 * - No locking is required as it's only accessed from the main thread
 */
class CGpuUploadLimiter
{
public:
	/**
	 * Maximum number of texture uploads allowed per frame.
	 * This value is tuned to balance loading speed vs frame smoothness.
	 */
	static constexpr int DEFAULT_MAX_UPLOADS_PER_FRAME = 30;

	/**
	 * Reset the upload counter for a new frame.
	 * Should be called at the beginning of each frame before main-thread upload consumers run.
	 */
	void OnFrameStart(int MaxUploadsPerFrame = DEFAULT_MAX_UPLOADS_PER_FRAME)
	{
		m_UploadsThisFrame = 0;
		m_MaxUploadsThisFrame = MaxUploadsPerFrame > 0 ? MaxUploadsPerFrame : 0;
	}

	/**
	 * Check if a new texture upload is allowed this frame.
	 * @return true if upload is allowed, false if the limit has been reached
	 */
	bool CanUpload() const { return CanUpload(1); }
	bool CanUpload(int Count) const { return Count >= 0 && m_UploadsThisFrame + Count <= m_MaxUploadsThisFrame; }

	/**
	 * Record that a texture upload has been performed.
	 * Should be called after each successful texture upload.
	 */
	void OnUploaded() { ++m_UploadsThisFrame; }

	/**
	 * Get the number of uploads performed this frame.
	 * Useful for debugging and statistics.
	 */
	int UploadsThisFrame() const { return m_UploadsThisFrame; }

	/**
	 * Get the maximum uploads per frame limit.
	 */
	static constexpr int DefaultMaxUploadsPerFrame() { return DEFAULT_MAX_UPLOADS_PER_FRAME; }
	int MaxUploadsPerFrame() const { return m_MaxUploadsThisFrame; }
	int RemainingUploads() const { return m_UploadsThisFrame < m_MaxUploadsThisFrame ? m_MaxUploadsThisFrame - m_UploadsThisFrame : 0; }

private:
	int m_UploadsThisFrame = 0;
	int m_MaxUploadsThisFrame = DEFAULT_MAX_UPLOADS_PER_FRAME;
};

#endif // ENGINE_CLIENT_GPU_UPLOAD_LIMITER_H
