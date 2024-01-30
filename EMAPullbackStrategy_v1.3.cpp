#include "sierrachart.h"

SCDLLName("EMAPullbackStrategy")

SCSFExport scsf_EMAPullbackStrategy(SCStudyInterfaceRef sc){
    
	// Define the persistent variables
	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[0];
	SCSubgraphRef SellEntrySubgraph = sc.Subgraph[1];
	
    SCSubgraphRef shortEMALine = sc.Subgraph[2];
    SCSubgraphRef longEMALine = sc.Subgraph[3];
    SCSubgraphRef trendEMALine = sc.Subgraph[4];

    SCInputRef shortEMAPeriod = sc.Input[0];
    SCInputRef longEMAPeriod = sc.Input[1];
    SCInputRef trendEMAPeriod = sc.Input[2];

    //SCInputRef rr = sc.Input[3];
    //SCInputRef riskLimit = sc.Input[4];
    //SCInputRef riskLimitHigh = sc.Input[5];

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
		
        shortEMALine.Name = "Short EMA";
        shortEMALine.DrawStyle = DRAWSTYLE_TRANSPARENT_FILL_TOP;
        shortEMALine.PrimaryColor = RGB(0, 255, 0);
        shortEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		shortEMALine.LineWidth = 2;
		shortEMALine.DrawZeros = false;

        longEMALine.Name = "Long EMA";
        longEMALine.DrawStyle = DRAWSTYLE_TRANSPARENT_FILL_BOTTOM;
        longEMALine.PrimaryColor = RGB(255, 0, 0);
        longEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		longEMALine.LineWidth = 2;
		longEMALine.DrawZeros = false;

        trendEMALine.Name = "Trend EMA";
        trendEMALine.DrawStyle = DRAWSTYLE_LINE;
        trendEMALine.PrimaryColor = RGB(0, 0, 255);
        trendEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		trendEMALine.LineWidth = 2;
		trendEMALine.DrawZeros = false;

		shortEMAPeriod.Name = "Short MA period";
		shortEMAPeriod.SetInt(7);
		longEMAPeriod.Name = "Long MA period";
		longEMAPeriod.SetInt(30);
		trendEMAPeriod.Name = "Trend MA period";
		trendEMAPeriod.SetInt(3);

		//rr.Name = "Take profit : Stop loss";
		//rr.SetFloat(1.5f);
		//riskLimit.Name = "Lowest risk per trade";
		//riskLimit.SetInt(10);
		//riskLimitHigh.Name = "Highest risk per trade";
		//riskLimitHigh.SetFloat(0.0002f);
		
		sessStartTime.Name = "Session start time";
		sessStartTime.SetTime(HMS_TIME(10, 0, 0));
		sessEndTime.Name = "Session end time";
		sessEndTime.SetTime(HMS_TIME(17 , 0, 0));
		
		sc.AllowMultipleEntriesInSameDirection = false;
		sc.MaximumPositionAllowed = 10;
		sc.SupportReversals = false;

		// This is false by default. Orders will go to the simulation system always.
		//sc.SendOrdersToTradeService = true;
		//sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = false;
		//sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = true;
		//sc.CancelAllWorkingOrdersOnExit = false;

		// Only 1 trade for each Order Action type is allowed per bar.
		sc.AllowOnlyOneTradePerBar = true;

		//This needs to be set to true when a trading study uses trading functions.
		//sc.MaintainTradeStatisticsAndTradesData = true;
		sc.AutoLoop = 1;
		sc.UpdateAlways = 1;
		sc.GraphRegion = 0;

		// During development set this flag to 1, so the DLL can be modified. When development is completed, set it to 0 to improve performance.
		//sc.FreeDLL = 0;
		
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

	bool inSession = true;

	if ((barDateTimeIn.GetHour() == sessStartDateTime.GetHour() && barDateTimeIn.GetMinute() >= sessStartDateTime.GetMinute()) ||
		(barDateTimeIn.GetHour() > sessStartDateTime.GetHour() && barDateTimeIn.GetHour() < sessEndDateTime.GetHour()) ||
		(barDateTimeIn.GetHour() == sessEndDateTime.GetHour() && barDateTimeIn.GetMinute() <= sessEndDateTime.GetMinute()))
	{
		inSession = true;
		//sc.AddMessageToLog("in session", 1);
	}
	else
	{
		inSession = false;
		//sc.AddMessageToLog("out session", 1);
	}

	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	bool buy;
	bool sell;

	if (sc.Index == 0){}// Only do this when calculating the first bar.
		
	if (sc.Index > 0){
		
		if (inSession){
			sc.ExponentialMovAvg(close, shortEMALine, shortEMAPeriod.GetInt());
			sc.ExponentialMovAvg(close, longEMALine, longEMAPeriod.GetInt());
			sc.ExponentialMovAvg(close, trendEMALine, trendEMAPeriod.GetInt());			
			
			float pricePullAboveEMA_maxClose = sc.GetPersistentFloat(1);
			float pricePullAboveEMA_maxHigh = sc.GetPersistentFloat(2);
			float pricePullBelowEMA_minClose = sc.GetPersistentFloat(3);
			float pricePullBelowMA_minLow = sc.GetPersistentFloat(4);
			
			/*if(shortEMA Line[sc.Index] > longEMALine[sc.Index] && longEMALine[sc.Index] > trendEMALine[sc.Index] && !buy) {
				BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];
				sell = true;
			}
			
			if(longEMALine[sc.Index] < longEMALine[sc.Index] && longEMALine[sc.Index] <trendEMALine[sc.Index] && !sell) {
				SellEntrySubgraph[sc.Index] = sc.High[sc.Index];
				buy = true;
			} */
			
			if(sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_BOTTOM) {
				BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];
			}
			
			if(sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_TOP) {
				SellEntrySubgraph[sc.Index] = sc.High[sc.Index];
			}
			
			/* if(sc.CrossOver(close, shortEMALine) == CROSS_FROM_BOTTOM)
			{
				sc.SetPersistentFloat(1, close[sc.Index]);
				sc.SetPersistentFloat(2, high[sc.Index]);
			}

			if(close[sc.Index] > pricePullAboveEMA_maxClose)
				sc.SetPersistentFloat(1, close[sc.Index]);
			if(high[sc.Index] > pricePullAboveEMA_maxHigh)
				sc.SetPersistentFloat(2, high[sc.Index]);

			if(sc.CrossOver(close, shortEMALine) == CROSS_FROM_TOP)
			{
				sc.SetPersistentFloat(3, close[sc.Index]);
				sc.SetPersistentFloat(4, low[sc.Index]);
			}
			    
			if(close[sc.Index] < pricePullBelowEMA_minClose)
				sc.SetPersistentFloat(3, close[sc.Index]);
			if(low[sc.Index] < pricePullBelowMA_minLow)
				sc.SetPersistentFloat(4, low[sc.Index]);


			bool long_strategy = sc.CrossOver(close, shortEMALine) == CROSS_FROM_BOTTOM && pricePullBelowEMA_minClose < longEMALine[sc.Index] && shortEMALine[sc.Index] > trendEMALine[sc.Index];
			bool short_strategy = sc.CrossOver(close, shortEMALine) == CROSS_FROM_TOP && pricePullAboveEMA_maxClose > longEMALine[sc.Index] && shortEMALine[sc.Index] < trendEMALine[sc.Index];
			//bool long_strategy = sc.CrossOver(close, shortEMALine) == CROSS_FROM_BOTTOM;
			//bool short_strategy = sc.CrossOver(close, shortEMALine) == CROSS_FROM_TOP;

			float risk_long;
			float risk_short;

			if (PositionData.PositionQuantity != 0 || PositionData.WorkingOrdersExist)
			{
				return;
			}	 */
				
			/* if (long_strategy)
			{
				risk_long = (close[sc.Index] - pricePullBelowMA_minLow) / close[sc.Index];
				if(risk_long > riskLimitLow.GetFloat())
				{
					BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];						
				}
				
			}

			if (short_strategy)
			{					
				risk_short = (pricePullAboveEMA_maxHigh - close[sc.Index]) / close[sc.Index];					
				if(risk_short > riskLimitHigh.GetFloat())
				{
					SellEntrySubgraph[sc.Index] = sc.High[sc.Index];				
				}
			}			 */	
			
		}
		else
		{
			if (PositionData.PositionQuantity != 0 || PositionData.WorkingOrdersExist)
			{				
				
			}
		}
	}
}
