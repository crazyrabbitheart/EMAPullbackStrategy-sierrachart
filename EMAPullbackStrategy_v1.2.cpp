#include "sierrachart.h"

SCDLLName("EMAPullbackStrategy")

SCSFExport scsf_EMAPullbackStrategy(SCStudyInterfaceRef sc)
{
    // Define the persistent variables
	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[0];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[1];
	SCSubgraphRef SellEntrySubgraph = sc.Subgraph[2];
	SCSubgraphRef SellExitSubgraph = sc.Subgraph[3];
	
    SCSubgraphRef shortEMALine = sc.Subgraph[4];
    SCSubgraphRef longEMALine = sc.Subgraph[5];
    SCSubgraphRef trendEMALine = sc.Subgraph[6];

    SCInputRef shortEMAPeriod = sc.Input[0];
    SCInputRef longEMAPeriod = sc.Input[1];
    SCInputRef trendEMAPeriod = sc.Input[2];

    SCInputRef rr = sc.Input[3];
    SCInputRef riskLimitLow = sc.Input[4];
    SCInputRef riskLimitHigh = sc.Input[5];

    SCInputRef sessStartTime = sc.Input[6];
    SCInputRef sessEndTime = sc.Input[7];
	
	SCString DebugMessage;
	
	int drawBuyBar = 0;
	int drawSellBar = 0;

    if (sc.SetDefaults)
    {
        sc.GraphName = "EMA Pullback Strategy";

		BuyEntrySubgraph.Name = "";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
        //BuyEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		//BuyEntrySubgraph.DrawZeros = false;

		BuyExitSubgraph.Name = "Buy Exit";
		BuyExitSubgraph.DrawStyle = DRAWSTYLE_ARROW_DOWN;
		BuyExitSubgraph.PrimaryColor = RGB(255, 128, 128);
		BuyExitSubgraph.LineWidth = 2;
        //BuyExitSubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		//BuyExitSubgraph.DrawZeros = false;

		SellEntrySubgraph.Name = "";
		SellEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_DOWN;
		SellEntrySubgraph.PrimaryColor = RGB(255, 0, 0);
		SellEntrySubgraph.LineWidth = 2;
        //SellEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		//SellEntrySubgraph.DrawZeros = false;	 

		SellExitSubgraph.Name = "Sell Exit";
		SellExitSubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		SellExitSubgraph.PrimaryColor = RGB(128, 255, 128);
		SellExitSubgraph.LineWidth = 2;
        //SellExitSubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		//SellExitSubgraph.DrawZeros = false;
		
        shortEMALine.Name = "Short EMA";
        shortEMALine.DrawStyle = DRAWSTYLE_LINE;
        shortEMALine.PrimaryColor = RGB(255, 0, 0);
        shortEMALine.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		shortEMALine.LineWidth = 2;
		shortEMALine.DrawZeros = false;

        longEMALine.Name = "Long EMA";
        longEMALine.DrawStyle = DRAWSTYLE_LINE;
        longEMALine.PrimaryColor = RGB(0, 255, 0);
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
		shortEMAPeriod.SetInt(33);
		longEMAPeriod.Name = "Long MA period";
		longEMAPeriod.SetInt(165);
		trendEMAPeriod.Name = "Trend MA period";
		trendEMAPeriod.SetInt(365);

		rr.Name = "Take profit : Stop loss";
		rr.SetFloat(1.5f);
		riskLimitLow.Name = "Lowest risk per trade";
		riskLimitLow.SetFloat(0.0005f);
		riskLimitHigh.Name = "Highest risk per trade";
		riskLimitHigh.SetFloat(0.0025f);
		
		sessStartTime.Name = "Session start time";
		sessStartTime.SetTime(HMS_TIME(9, 0, 0));
		sessEndTime.Name = "Session end time";
		sessEndTime.SetTime(HMS_TIME(15 , 0, 0));
		
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

	sc.ExponentialMovAvg(close, shortEMALine, shortEMAPeriod.GetInt());
	sc.ExponentialMovAvg(close, longEMALine, longEMAPeriod.GetInt());
	sc.ExponentialMovAvg(close, trendEMALine, trendEMAPeriod.GetInt());
	
	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	
	float pricePullAboveEMA_maxClose = sc.GetPersistentFloat(1);
	float pricePullAboveEMA_maxHigh = sc.GetPersistentFloat(2);
	float pricePullBelowEMA_minClose = sc.GetPersistentFloat(3);
	float pricePullBelowMA_minLow = sc.GetPersistentFloat(4);

	if (sc.Index == 0)// Only do this when calculating the first bar.
	{
	}
	
	if (sc.Index > 0)
	{
		if (inSession)
		{
			if(sc.CrossOver(close, shortEMALine) == CROSS_FROM_BOTTOM)
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
				
			}

			sc.AddMessageToLog("////Here", 1);
			DebugMessage.Format("pricePullBelowEMA_minClose is %f", pricePullBelowEMA_minClose);
			sc.AddMessageToLog(DebugMessage, 1);
			DebugMessage.Format("pricePullAboveEMA_maxClose is %f", pricePullAboveEMA_maxClose);
			sc.AddMessageToLog(DebugMessage, 1);
			if (long_strategy)
			{
				sc.AddMessageToLog("////Buy Case", 1);
			    risk_long = (close[sc.Index] - pricePullBelowMA_minLow) / close[sc.Index];
				DebugMessage.Format("risk_long is %f", risk_long);
				sc.AddMessageToLog(DebugMessage, 1);
    			if(risk_long > riskLimitLow.GetFloat())
    			{
					//BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];
					sc.AddMessageToLog("////Buy", 1);
					s_SCNewOrder NewOrder;
					NewOrder.OrderQuantity = 1;
					NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
					NewOrder.Target1Price = close[sc.Index] * (1 + rr.GetFloat() * risk_long);
					NewOrder.Stop1Price = close[sc.Index] * (1 - risk_long);
					sc.BuyEntry(NewOrder);
    			}
				
			}

			if (short_strategy)
			{
				sc.AddMessageToLog("////Sell Case", 1);
			    risk_short = (pricePullAboveEMA_maxHigh - close[sc.Index]) / close[sc.Index];
				DebugMessage.Format("risk_short is %f", risk_short);
				sc.AddMessageToLog(DebugMessage, 1);
    			if(risk_short > riskLimitHigh.GetFloat())
    			{
					sc.AddMessageToLog("////Sell", 1);
					s_SCNewOrder NewOrder;
					NewOrder.OrderQuantity = 1;
					NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
					NewOrder.Target1Price = close[sc.Index] * (1 - rr.GetFloat() * risk_short);
					NewOrder.Stop1Price = close[sc.Index] * (1 + risk_short);
					sc.SellEntry(NewOrder);
    			}
			}
		}
		else
		{
			if (PositionData.PositionQuantity != 0 || PositionData.WorkingOrdersExist)
			{				
				sc.FlattenAndCancelAllOrders();
			}
		}
	}
}
