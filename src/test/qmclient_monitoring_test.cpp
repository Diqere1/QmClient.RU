#include <game/client/components/qmclient/monitoring.h>

#include <gtest/gtest.h>

TEST(QmMonitoringHelpers, ConnectionGradeTracksDisconnectedState)
{
	SQmNetworkMetrics Net;
	Net.m_Connected = false;
	EXPECT_EQ(QmDetermineConnectionGrade(Net), EQmConnectionGrade::DISCONNECTED);
}

TEST(QmMonitoringHelpers, ConnectionGradeUsesThresholdTable)
{
	SQmNetworkMetrics Net;
	Net.m_Connected = true;
	Net.m_SnapshotLatencyMs = 40.0f;
	Net.m_PredictionLatencyMs = 50.0f;
	Net.m_JitterMs = 5.0f;
	Net.m_PacketLossPct = 0.0f;
	EXPECT_EQ(QmDetermineConnectionGrade(Net), EQmConnectionGrade::NORMAL);

	Net.m_PredictionLatencyMs = 110.0f;
	EXPECT_EQ(QmDetermineConnectionGrade(Net), EQmConnectionGrade::ELEVATED);

	Net.m_PredictionLatencyMs = 210.0f;
	EXPECT_EQ(QmDetermineConnectionGrade(Net), EQmConnectionGrade::SEVERE);
}

TEST(QmMonitoringHelpers, PrimaryCausePrefersDominantMetric)
{
	SQmNetworkMetrics Net;
	SQmPerformanceMetrics Perf;

	Net.m_Connected = false;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::DISCONNECTED), EQmDiagnosticCause::NONE);

	Net.m_Connected = true;
	Net.m_SnapshotLatencyMs = 120.0f;
	Net.m_PredictionLatencyMs = 40.0f;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::ELEVATED), EQmDiagnosticCause::DOWNSTREAM);

	Net.m_SnapshotLatencyMs = 20.0f;
	Net.m_PredictionLatencyMs = 95.0f;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::ELEVATED), EQmDiagnosticCause::UPSTREAM);

	Net.m_PredictionLatencyMs = 30.0f;
	Net.m_JitterMs = 28.0f;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::SEVERE), EQmDiagnosticCause::JITTER);

	Net.m_JitterMs = 6.0f;
	Net.m_PacketLossPct = 8.0f;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::SEVERE), EQmDiagnosticCause::PACKET_LOSS);

	Net.m_PacketLossPct = 0.0f;
	Net.m_ConnectionProblems = true;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::SEVERE), EQmDiagnosticCause::DOWNSTREAM);

	Net.m_PacketLossPct = 2.0f;
	Net.m_ConnectionProblems = true;
	Net.m_SnapshotLatencyMs = 130.0f;
	Net.m_PredictionLatencyMs = 20.0f;
	EXPECT_EQ(QmDeterminePrimaryCause(Net, Perf, EQmConnectionGrade::ELEVATED), EQmDiagnosticCause::PACKET_LOSS);
}

TEST(QmMonitoringHelpers, DiagnosticLossRateUsesSendDeltaAndResends)
{
	EXPECT_NEAR(QmComputeDiagnosticPacketLossPct(60, 20), 33.3333f, 0.001f);
	EXPECT_FLOAT_EQ(QmComputeDiagnosticPacketLossPct(60, 20), 33.333332f);
	EXPECT_FLOAT_EQ(QmComputeDiagnosticPacketLossPct(0, 0), 0.0f);
	EXPECT_FLOAT_EQ(QmComputeDiagnosticPacketLossPct(0, 3), 100.0f);
}

TEST(QmMonitoringHelpers, RollbackAmountUsesNegativeGameTimeMargin)
{
	EXPECT_FLOAT_EQ(QmComputeRollbackMs(-18.0f), 18.0f);
	EXPECT_FLOAT_EQ(QmComputeRollbackMs(6.0f), 0.0f);
}

TEST(QmMonitoringHelpers, PeakSelectionPrefersLatestMatchingPeak)
{
	std::array<float, 8> aHistory = {41.0f, 39.0f, 41.0f, 24.0f, 24.0f, 18.0f, 24.0f, 17.0f};
	EXPECT_EQ(QmFindLatestPeakIndex(aHistory, 0, 8), 2);
	EXPECT_EQ(QmFindLatestAbsolutePeakIndex(aHistory, 0, 8), 2);
}

TEST(QmMonitoringHelpers, SignedPeakSelectionUsesLatestAbsolutePeak)
{
	std::array<float, 8> aHistory = {-7.0f, 5.0f, -9.0f, 3.0f, 9.0f, 4.0f, 8.0f, 2.0f};
	EXPECT_EQ(QmFindLatestAbsolutePeakIndex(aHistory, 0, 8), 4);
}

TEST(QmMonitoringHelpers, UiScaleGrowsOnHighResolutionScreens)
{
	EXPECT_FLOAT_EQ(QmComputeMonitoringUiScale(800.0f, 600.0f), 0.65f);
	const float Expected1600x900 = std::sqrt((1600.0f / 1920.0f) * (900.0f / 1080.0f));
	EXPECT_FLOAT_EQ(QmComputeMonitoringUiScale(1600.0f, 900.0f), Expected1600x900);
	EXPECT_FLOAT_EQ(QmComputeMonitoringUiScale(3840.0f, 2160.0f), 1.8f);
}

TEST(QmMonitoringHelpers, HudLayoutPlacesPanelLeftOfGraphColumn)
{
	const SQmMonitoringHudLayout Layout = QmComputeMonitoringHudLayout(1600.0f, 900.0f, 1184.0f, 16.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.w, 768.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.h, 702.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.x, 400.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.y, 32.0f);
	EXPECT_FLOAT_EQ(Layout.m_ContentRect.x, 412.0f);
	EXPECT_FLOAT_EQ(Layout.m_ContentRect.y, 44.0f);
}

TEST(QmMonitoringHelpers, BodyLayoutPreservesMetricsBudgetOnCompactPanels)
{
	const SQmMonitoringBodyLayout Layout = QmComputeMonitoringBodyLayout(260.0f, 1.0f);
	EXPECT_NEAR(Layout.m_MainGraphHeight, 55.7f, 0.1f);
	EXPECT_NEAR(Layout.m_FpsGraphHeight, 35.6f, 0.1f);
	EXPECT_NEAR(Layout.m_PrimaryCardsHeight, 35.6f, 0.1f);
	EXPECT_NEAR(Layout.m_SecondaryCardsHeight, 33.2f, 0.1f);
	EXPECT_GT(Layout.m_PrimaryCardsHeight, 30.0f);
	EXPECT_GT(Layout.m_SecondaryCardsHeight, 30.0f);
}

TEST(QmMonitoringHelpers, HudLayoutUsesLargerPanelOn4kScreens)
{
	const SQmMonitoringHudLayout Layout = QmComputeMonitoringHudLayout(3840.0f, 2160.0f, 2842.0f, 38.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.w, 1843.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.h, 1685.0f);
	EXPECT_FLOAT_EQ(Layout.m_ContentRect.x, 986.0f);
	EXPECT_FLOAT_EQ(Layout.m_ContentRect.y, 101.0f);
}

TEST(QmMonitoringHelpers, HudLayoutClampsPanelInsideScreenBounds)
{
	const SQmMonitoringHudLayout Layout = QmComputeMonitoringHudLayout(360.0f, 240.0f, 120.0f, 16.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.x, 0.0f);
	EXPECT_FLOAT_EQ(Layout.m_PanelRect.y, 0.0f);
	EXPECT_LE(Layout.m_PanelRect.x + Layout.m_PanelRect.w, 360.0f);
	EXPECT_LE(Layout.m_PanelRect.y + Layout.m_PanelRect.h, 240.0f);
}
