//**|***********************************************************|
//**|description about EMAPullbackStrategy v1.6.				|
//**|															|
//**|It has two EMAs.											|
//**|Find out highest points between crossovers of two EMAs.	|
//**|Draw rectangle with white color at the points.				|
//**|***********************************************************|

#include "sierrachart.h"

SCDLLName("EMAPullbackStrategy")

SCSFExport scsf_EMAPullbackStrategy(SCStudyInterfaceRef sc){
    
	// Define the persistent variables
	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[0];
	SCSubgraphRef SellEntrySubgraph = sc.Subgraph[1];	
    SCSubgraphRef shortEMALine = sc.Subgraph[2];
    SCSubgraphRef longEMALine = sc.Subgraph[3];    
	SCSubgraphRef existSignal = sc.Subgraph[4];
	SCSubgraphRef sessionSignal = sc.Subgraph[5];	// if signal is in session, return 1, else 0.
	SCSubgraphRef Subgraph_Background   = sc.Subgraph[6]; 
	SCSubgraphRef crossover   = sc.Subgraph[7]; 
	
    SCInputRef shortEMAPeriod = sc.Input[0];
    SCInputRef longEMAPeriod = sc.Input[1];
    SCInputRef threshold = sc.Input[2];
    SCInputRef sessStartTime = sc.Input[3];
    SCInputRef sessEndTime = sc.Input[4];
	SCInputRef lineWidth = sc.Input[5];
	SCInputRef buyColor = sc.Input[6];
	SCInputRef sellColor = sc.Input[7];
	
	SCString DebugMessage;
	
    if (sc.SetDefaults) {
		
        sc.GraphName = "EMA Pullback Strategy";

		crossover.Name = "cross";
		crossover.DrawStyle = DRAWSTYLE_ARROW_UP;
		crossover.PrimaryColor = RGB(255, 255, 255);
		crossover.LineWidth = 3;
        crossover.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		crossover.DrawZeros = false;
		
		BuyEntrySubgraph.Name = " ";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 3;
        BuyEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		BuyEntrySubgraph.DrawZeros = false;

		SellEntrySubgraph.Name = " ";
		SellEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_DOWN;
		SellEntrySubgraph.PrimaryColor = RGB(255, 0, 0);
		SellEntrySubgraph.LineWidth = 3;
        SellEntrySubgraph.LineLabel = LL_DISPLAY_NAME | LL_NAME_ALIGN_RIGHT;
		SellEntrySubgraph.DrawZeros = false;	 
		
        shortEMALine.Name = "price Line";
        shortEMALine.DrawStyle = DRAWSTYLE_LINE;
        shortEMALine.PrimaryColor = RGB(255,160,10);
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
		sessEndTime.SetTime(HMS_TIME(17 , 0, 0));
		
		lineWidth.Name = "Rectangle Line Width";
		lineWidth.SetInt(3);
		
		buyColor.Name = "Buy Color";
		buyColor.SetColor(200,200,0);
		sellColor.Name = "Sell Color";
		sellColor.SetColor(0,200,200);
		
		sc.AllowMultipleEntriesInSameDirection = false;
		sc.MaximumPositionAllowed = 10;
		sc.SupportReversals = false;
		sc.SupportAttachedOrdersForTrading = false;		
		sc.AllowEntryWithWorkingOrders = true;
		sc.AllowOnlyOneTradePerBar = true;
		sc.AutoLoop = 1;
		sc.UpdateAlways = 1;
		sc.GraphRegion = 0;
		
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
	} else {
		inSession = false;		
	}

	if (sc.Index == 0){
		existSignal[sc.Index] = 0;
		sessionSignal[sc.Index] = 0;
		return;
	}// Only do this when calculating the first bar.
	
	if (sc.Index > 0){
		
		existSignal[sc.Index] = existSignal[sc.Index - 1];
		
		if (inSession) {
			sessionSignal[sc.Index] = 1;
			Subgraph_Background[sc.Index] = 1;
		} else
			sessionSignal[sc.Index] = 0;

		if(sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_BOTTOM) {
			existSignal[sc.Index] = 1;
			//crossover[sc.Index] = sc.Low[sc.Index];
			
			if(existSignal[sc.Index-1] == -1) return;
			
			//find points for highest price
			int i = sc.Index;
			int lowestPoint = 0;
			int lowestValue = close[sc.Index];
			int barCount = 0;
			int rangeStart = 0;
			int rangeEnd = 0;
			float crossoverValue = close[sc.Index];
								
			while(existSignal[i] != -1 && i >= 0){				
				if (lowestValue > close[i]){
					lowestPoint = i;
					lowestValue = close[i];				
				}			
				barCount++;
				i--;
			}
			
			i = lowestPoint;
			while(existSignal[i] != -1 && i >= 0){
				if (close[i] > crossoverValue) {
					rangeStart = i;
					break;
				}
				if (sessionSignal[i] == 0 && rangeStart == 0){
					rangeStart = i;
					break;
				}
				i--;
			}
			if (rangeStart == 0) rangeStart = i;  // if there isn`t point to equal crossoverValue, it`s previous crossoverValue
			
			i = lowestPoint;
			while(existSignal[i] != 1 && i <= sc.Index){
				if (close[i] > crossoverValue) {
					rangeEnd = i;
					break;
				}
				if (sessionSignal[i] == 0 && rangeEnd == 0){
					rangeEnd = i;
					break;
				}
				i++;
			}
			if (rangeEnd == 0) rangeEnd = i;  // if there isn`t point to equal crossoverValue, it`s previous crossoverValue
			
			//crossover[lowestPoint] = low[lowestPoint];
			
			//draw rectangle for highest point
			
			if (barCount < 3) return; //if distance between overcrosses is short, ignore.
			if ((longEMALine[lowestPoint] - shortEMALine[lowestPoint]) < threshold.GetFloat()) return; //if highest value is less than threshold, ignore.
			
			int Transparency_Level = 60; 
			int UniqueLineNumber = 0;
			
			s_UseTool Tool; 
			Tool.Clear();  
			Tool.ChartNumber = sc.ChartNumber; 
			Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT; 
			Tool.LineNumber =  UniqueLineNumber + sc.Index; 
			Tool.BeginDateTime = sc.BaseDateTimeIn[lowestPoint]; 
			Tool.BeginIndex = rangeStart; 
			Tool.BeginValue = sc.Close[lowestPoint] + 0.02/sc.BaseGraphScaleValueOffset; 
			Tool.EndIndex = rangeEnd; 
			Tool.EndValue = sc.Close[lowestPoint] - 0.02/sc.BaseGraphScaleValueOffset; 
			Tool.Color = buyColor.GetColor();
			Tool.TransparencyLevel = Transparency_Level;  
			Tool.LineWidth = lineWidth.GetInt(); 
			sc.UseTool(Tool); 			
			
		}
			
		if(sc.CrossOver(shortEMALine, longEMALine) == CROSS_FROM_TOP) {
			//crossover[sc.Index] = sc.Low[sc.Index];
			existSignal[sc.Index] = -1;
			
			if(existSignal[sc.Index-1] == 1) return;
			
			//find points for highest price
			int i = sc.Index;
			int highestPoint = 0;
			int highestValue = 0;
			int barCount = 0;
			int rangeStart = 0;
			int rangeEnd = 0;
			float crossoverValue = close[sc.Index];
								
			while(existSignal[i] != 1 && i >= 0){
				
				if (highestValue < close[i]){
					highestPoint = i;
					highestValue = close[i];				
				}			
				barCount++;
				i--;
			}
			
			i = highestPoint;
			while(existSignal[i] != 1 && i >= 0){
				if (close[i] < crossoverValue) {
					rangeStart = i;
					break;
				}
				if (sessionSignal[i] == 0 && rangeStart == 0){
					rangeStart = i;
					break;
				}
				i--;
			}
			if (rangeStart == 0) rangeStart = i;  // if there isn`t point to equal crossoverValue, it`s previous crossoverValue
			
			i = highestPoint;
			while(existSignal[i] != -1 && i <= sc.Index){
				if (close[i] < crossoverValue) {
					rangeEnd = i;
					break;
				}
				if (sessionSignal[i] == 0 && rangeEnd == 0){
					rangeEnd = i;
					break;
				}
				i++;
			}
			if (rangeEnd == 0) rangeEnd = i;  // if there isn`t point to equal crossoverValue, it`s previous crossoverValue
			
			//crossover[highestPoint] = high[highestPoint];
			
			//draw rectangle for highest point
			
			if (barCount < 3) return; //if distance between overcrosses is short, ignore.
			if ((shortEMALine[highestPoint] - longEMALine[highestPoint]) < threshold.GetFloat()) return; //if highest value is less than threshold, ignore.
			
			int Transparency_Level = 60; 
			int UniqueLineNumber = 0;		

			/* SCString DebugMessage;
			DebugMessage.Format("%d,%d", rangeStart, rangeEnd);
			sc.AddMessageToLog(DebugMessage, 1); */

			s_UseTool Tool; 
			Tool.Clear();  
			Tool.ChartNumber = sc.ChartNumber; 
			Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT; 
			Tool.LineNumber =  UniqueLineNumber + sc.Index; 
			Tool.BeginDateTime = sc.BaseDateTimeIn[highestPoint]; 
			Tool.BeginIndex = rangeStart; 
			Tool.BeginValue = sc.Close[highestPoint] + 0.02/sc.BaseGraphScaleValueOffset; 
			Tool.EndIndex = rangeEnd; 
			Tool.EndValue = sc.Close[highestPoint] - 0.02/sc.BaseGraphScaleValueOffset; 
			Tool.Color = sellColor.GetColor(); 
			Tool.TransparencyLevel = Transparency_Level;  
			Tool.LineWidth = lineWidth.GetInt(); 
			sc.UseTool(Tool); 
		}
			
			
		if(shortEMALine[sc.Index] < longEMALine[sc.Index]) {		
			if (existSignal[sc.Index - 1] == -1) {					
				existSignal[sc.Index] = -2;
			}
		}
		
		if(shortEMALine[sc.Index] > longEMALine[sc.Index]) {				
			if (existSignal[sc.Index - 1] == 1) {					
				existSignal[sc.Index] = 2;			
			}
		}		
	}	
}