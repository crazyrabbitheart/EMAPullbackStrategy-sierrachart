#include "sierrachart.h"

SCDLLName("EMAPullbackStrategy")

SCSFExport scsf_EMAPullbackStrategy(SCStudyInterfaceRef sc) {

	// Define the persistent variables
	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[0];
	SCSubgraphRef SellEntrySubgraph = sc.Subgraph[1];
	SCSubgraphRef shortEMALine = sc.Subgraph[2];
	SCSubgraphRef longEMALine = sc.Subgraph[3];
	SCSubgraphRef existSignal = sc.Subgraph[4];
	SCSubgraphRef highSignal = sc.Subgraph[5];
	SCSubgraphRef Subgraph_Background = sc.Subgraph[6];

	SCInputRef shortEMAPeriod = sc.Input[0];
	SCInputRef longEMAPeriod = sc.Input[1];
	SCInputRef threshold = sc.Input[2];

	SCInputRef sessStartTime = sc.Input[6];
	SCInputRef sessEndTime = sc.Input[7];

	SCString DebugMessage;

	if (sc.SetDefaults) {

		sc.GraphName = "EMA Pullback Strategy";

		BuyEntrySubgraph.Name = "BUY_POINT";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 3;
		BuyEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		BuyEntrySubgraph.DrawZeros = false;

		SellEntrySubgraph.Name = "SELL_POINT";
		SellEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_DOWN;
		SellEntrySubgraph.PrimaryColor = RGB(255, 0, 0);
		SellEntrySubgraph.LineWidth = 3;
		SellEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		SellEntrySubgraph.DrawZeros = false;

		shortEMALine.Name = "price Line";
		shortEMALine.DrawStyle = DRAWSTYLE_LINE;
		shortEMALine.PrimaryColor = RGB(0, 255, 0);
		shortEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		shortEMALine.LineWidth = 2;
		shortEMALine.DrawZeros = false;

		longEMALine.Name = "EMA Line";
		longEMALine.DrawStyle = DRAWSTYLE_LINE;
		longEMALine.PrimaryColor = RGB(255, 255, 255);
		longEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		longEMALine.LineWidth = 2;
		longEMALine.DrawZeros = false;

		Subgraph_Background.Name = "Background";
		Subgraph_Background.DrawStyle = DRAWSTYLE_BACKGROUND_TRANSPARENT;
		Subgraph_Background.PrimaryColor = COLOR_WHITE;
		Subgraph_Background.SecondaryColor = COLOR_LIGHTPINK;
		Subgraph_Background.SecondaryColorUsed = true;        // turn on if both colors are to be used 
		Subgraph_Background.AutoColoring = AUTOCOLOR_POSNEG;  // use positive/negative values to signify colors 

		shortEMAPeriod.Name = "Short MA period";
		shortEMAPeriod.SetInt(1);
		longEMAPeriod.Name = "Long MA period";
		longEMAPeriod.SetInt(20);
		threshold.Name = "threshold";
		threshold.SetFloat(1.0);

		sessStartTime.Name = "Session start time";
		sessStartTime.SetTime(HMS_TIME(10, 0, 0));
		sessEndTime.Name = "Session end time";
		sessEndTime.SetTime(HMS_TIME(17, 0, 0));

		sc.AllowMultipleEntriesInSameDirection = false;
		sc.MaximumPositionAllowed = 10;
		sc.SupportReversals = false;
		sc.SupportAttachedOrdersForTrading = false;
		sc.AllowEntryWithWorkingOrders = true;
		sc.AllowOnlyOneTradePerBar = true;
		sc.AutoLoop = 1;
		sc.UpdateAlways = 1;
		sc.GraphRegion = 0;
		sc.DrawStudyUnderneathMainPriceGraph = 1;    // not required in studies, but want color behind price for this study 


		return;
	}

	if (sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_NOT_CLOSED)		return;

	SCFloatArrayRef high = sc.High;
	SCFloatArrayRef low = sc.Low;
	SCFloatArrayRef open = sc.Open;
	SCFloatArrayRef close = sc.Close;

	SCDateTime barDateTimeIn = sc.BaseDateTimeIn[sc.Index];
	SCDateTime sessStartDateTime(0, sessStartTime.GetTime());
	SCDateTime sessEndDateTime(0, sessEndTime.GetTime());

	sc.ExponentialMovAvg(close, shortEMALine, shortEMAPeriod.GetInt());
	sc.ExponentialMovAvg(close, longEMALine, longEMAPeriod.GetInt());

	bool inSession = true;

	if ((barDateTimeIn.GetHour() == sessStartDateTime.GetHour() && barDateTimeIn.GetMinute() >= sessStartDateTime.GetMinute()) ||
		(barDateTimeIn.GetHour() > sessStartDateTime.GetHour() && barDateTimeIn.GetHour() < sessEndDateTime.GetHour()) ||
		(barDateTimeIn.GetHour() == sessEndDateTime.GetHour() && barDateTimeIn.GetMinute() <= sessEndDateTime.GetMinute())) {
		inSession = true;
	}
	else {
		inSession = false;
	}

	if (sc.Index == 0) {}// Only do this when calculating the first bar.

	existSignal[sc.Index] = existSignal[sc.Index - 1];

	if (sc.Index > 0) {

		if (inSession) {

			if (sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_BOTTOM) {
				//if(shortEMALine[sc.Index] > longEMALine[sc.Index] && shortEMALine[sc.Index-1] < longEMALine[sc.Index-1]) {
				existSignal[sc.Index] = 1;
			}

			if (sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_TOP) {
				//if(shortEMALine[sc.Index] < longEMALine[sc.Index] && shortEMALine[sc.Index-1] > longEMALine[sc.Index-1]) {
					//SellEntrySubgraph[sc.Index] = sc.High[sc.Index];
				existSignal[sc.Index] = -1;

				//draw background for highest price
				int i = sc.Index;
				int highestPoint = 0;
				int highestValue = 0;
				int j = 0;
				SCString Buffer;
				while (existSignal[i] != 1) {
					if (highestValue < close[i]) {
						highestPoint = i;
						highestValue = close[i];
						j++;
					}
					i--;
				}

				for (int k = -j / 2 - 1; k <= j / 2 + 1; k++) {
					Subgraph_Background[highestPoint + k] = 1;
				}


				/* if(j<=5)
					Subgraph_Background[highestPoint] = 1;    // use primary color
				else if(j>5){
					for (int k=-1;k<2;k++){
						Subgraph_Background[highestPoint + k] = 1;    // use primary color
					}
				}else if(j>8 && j>5){
					for (int k=-2;k<3;k++){
						Subgraph_Background[highestPoint + k] = 1;    // use primary color
					}
				} */
			}


			if (shortEMALine[sc.Index] < longEMALine[sc.Index]) {
				if (existSignal[sc.Index - 1] == -1) {
					existSignal[sc.Index] = -2;
					return;
				}
				if (((longEMALine[sc.Index] - shortEMALine[sc.Index]) > threshold.GetFloat()) && (existSignal[sc.Index] == -2)) {
					BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];
					existSignal[sc.Index] = 0;
				}
			}

			if (shortEMALine[sc.Index] > longEMALine[sc.Index]) {
				if (existSignal[sc.Index - 1] == 1) {
					existSignal[sc.Index] = 2;
					return;
				}
				if (((shortEMALine[sc.Index] - longEMALine[sc.Index]) > threshold.GetFloat()) && (existSignal[sc.Index] == 2)) {
					SellEntrySubgraph[sc.Index] = sc.High[sc.Index];
					existSignal[sc.Index] = 0;
				}
			}
		}
	}
}