var ThisSheet;
var map;
var ThisRow;
var LastPointTime;
var ThisPointTime;
var Hotspots;

// Run once sheet is open
function onOpen(){
  ThisRow=2;
  // Resize columns width
  ThisSheet = SpreadsheetApp.getActiveSheet().setColumnWidths(1, 8, 85);
  // Remove all map images
  ThisSheet.getImages().forEach(function(i){i.remove()});
  // Keep text in cells
  ThisSheet.getRange('A:H').setWrapStrategy(SpreadsheetApp.WrapStrategy.CLIP);
  var Seq=1;
  ThisPointTime=ThisSheet.getRange(ThisRow,1).getValue();
  while (ThisPointTime != ''){
    // Start map caption
    ThisSheet.getRange(((Seq-1)*30)+27, 10).setValue('Starting at row '+ThisRow);
    // Create a map
    map = Maps.newStaticMap();
    // First marker
    PlaceMarker(Maps.StaticMap.MarkerSize.SMALL, "0x00FF00", 'Green');
    // The difference between this point and the last one is less than 10 minutes
    while (ThisPointTime - LastPointTime < 600000) {
      // Found a dog
      if (ThisSheet.getRange(ThisRow-1,1).getBackgroundObject().asRgbColor().asHexString()=='#ff00ff'){
         map.setMarkerStyle(Maps.StaticMap.MarkerSize.SMALL, "0xFF00FF", 'Magenta');
         map.addMarker(ThisSheet.getRange(ThisRow-1,2).getValue(), ThisSheet.getRange(ThisRow-1,3).getValue());
      }
      // Is there a next marker or last one?
      (ThisSheet.getRange(ThisRow+1,1).getValue() - LastPointTime < 600000)? PlaceMarker(Maps.StaticMap.MarkerSize.TINY, "0x0000FF", 'Blue'): PlaceMarker(Maps.StaticMap.MarkerSize.SMALL, "0xFF0000", 'Red');
    }
    // Add GPS track image to sheet
    ThisSheet.insertImage(Utilities.newBlob(map.getMapImage(), 'image/png', Seq), 10, ((Seq-1)*30)+2);
    // End map caption
    ThisSheet.getRange(((Seq-1)*30)+27, 10).setValue(ThisSheet.getRange(((Seq-1)*30)+27, 10).getValue() + ' ending at row ' + (ThisRow-1)).setFontWeight("bold");
    Seq++;
  }
}

function PlaceMarker(a,b,c) {
  map.setMarkerStyle(a,b,c);
  map.addMarker(ThisSheet.getRange(ThisRow,2).getValue(), ThisSheet.getRange(ThisRow,3).getValue());
  LastPointTime=ThisPointTime;
  ThisRow++;
  ThisPointTime=ThisSheet.getRange(ThisRow,1).getValue();
}

// Run once HTTP post comes from Helium
function doPost(e) { 
  var GS = SpreadsheetApp.openById('');
  var SheetDate = new Date().toLocaleDateString();
  // Create a sheet for today if it doesn't exist and add column headers
  if (!GS.getSheetByName(SheetDate)) GS.insertSheet(SheetDate).getRange('A1:H1').setValues([['Time','Latitude','Longitude','Altitude','Speed','Hotspots','SNR','RSSI']]).setFontWeight("bold");
  ThisSheet = GS.getSheetByName(SheetDate);
  GS.setActiveSheet(ThisSheet);
  // Make it the first sheet
  GS.moveActiveSheet(1);
  // Row array
  var ThisRecord = [];
  // Timestamp
  ThisRecord[0] = new Date().toLocaleTimeString();
  // Get all contents
  var json = JSON.parse(e.postData.contents);
  // Get all hotspots
  Hotspots=json.hotspots;
  // Decoded data
  ThisRecord[1]=json.decoded.payload[0].value.latitude;
  ThisRecord[2]=json.decoded.payload[0].value.longitude;
  ThisRecord[3]=json.decoded.payload[0].value.altitude;
  ThisRecord[4]=json.decoded.payload[1].value;
  ThisRecord[5]=GetHotspot('name');
  ThisRecord[6]=GetHotspot('snr');
  ThisRecord[7]=GetHotspot('rssi');
  // Save to spreadsheet
  ThisSheet.getRange(ThisSheet.getLastRow() + 1, 1, 1, ThisRecord.length).setValues([ThisRecord]);
  if (json.decoded.payload[2].value==1) ThisSheet.getRange(ThisSheet.getLastRow(), 1, 1, ThisRecord.length).setBackground("magenta");
}

// Get a property of hotspot
function GetHotspot(Prop) {
  var TempText='';
  var HotspotNumber=-1;
  // Build CSV
  while (Hotspots[++HotspotNumber]) TempText=TempText+eval('Hotspots[HotspotNumber].'+Prop)+',';
  // Remove last comma
  return TempText.substring(0, TempText.length - 1);
}
