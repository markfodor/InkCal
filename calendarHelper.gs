const daysToViewAhead = 1;    // The days ahead you wish to dispay, ideally it is only the current day so 1
const dateFormatPattern = 'yyyy-MM-dd';
const timePattern = 'HH:mm';

function doGet(e) {
  // E.g: 'Europe/Madrid' -> you can override this if you want to differ from your Google account setup
  // https://developers.google.com/google-ads/api/data/codes-formats#timezone-ids
  const timeZone = Session.getScriptTimeZone();
  const calendars = CalendarApp.getAllOwnedCalendars();

  if (calendars == undefined) {
    const error = "Can not access any calendar";
    Logger.log(error);
    return ContentService.createTextOutput({ "error": error }).setMimeType(ContentService.MimeType.JSON);
  }

  let start = new Date(); 
  start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + daysToViewAhead * oneday);
  let events = mergeCalendarEvents(calendars, start, stop); //pull start/stop time

  // exclude the INVITED and NO answered events --> include only your own events and the ones you answared YES or MAYBE
  events = events.filter(event =>
    event.getMyStatus() == CalendarApp.GuestStatus.OWNER ||
    event.getMyStatus() == CalendarApp.GuestStatus.YES ||
    event.getMyStatus() == CalendarApp.GuestStatus.MAYBE
  );

  eventArray = [];
  for (let i = 0; i < events.length; i++) {
    eventArray.push({ 
      "name": removeAccentsFromString(events[i].getTitle()),
      "allDayEvent" : events[i].isAllDayEvent(),
      "startDate": Utilities.formatDate(events[i].getStartTime(), timeZone, timePattern), // TODO rename these to startTime, endTime
      "endDate": Utilities.formatDate(events[i].getEndTime(), timeZone, timePattern)
    });
  }

  const currentTime = getFormattedTimestamp(timeZone, timePattern);
  const respoonse = {
    "date": getFormattedTimestamp(timeZone, dateFormatPattern),
    "time": currentTime,
    "sleep": calculateDeepSleepInMins(eventArray),
    "events": eventArray
  };
  const jsonString = JSON.stringify(respoonse); 

  // uncomment this line for testing
  Logger.log(jsonString);
  return ContentService.createTextOutput(jsonString).setMimeType(ContentService.MimeType.JSON);
}

function calculateDeepSleepInMins(eventArray) {
  const minsUntilMidnight = getMinutesUntilMidnight();
  const minsUntilNextEvent = 99999;
  
  for (let i = 0; i < eventArray.length; i++) {
    if (!eventArray[i].allDayEvent) {
      const minsUntilNext = getMinutesUntilTime(eventArray[i].startDate);
      if (minsUntilNext < minsUntilNextEvent) {
        minsUntilNextEvent = minsUntilNext;
      }
    }
  }

  return (minsUntilMidnight < minsUntilNextEvent) ? minsUntilMidnight : minsUntilNextEvent;
}

function getMinutesUntilTime(targetTimeString) {
  const now = new Date();
  const [targetHours, targetMinutes] = targetTimeString.split(":").map(Number);
  const targetTime = new Date();
  targetTime.setHours(targetHours, targetMinutes, 0, 0); // Set target hours and minutes

  // If the target time is earlier than the current time
  if (targetTime < now) {
    return 99999;
  }

  // Calculate the difference in milliseconds and convert to minutes
  const difference = targetTime - now;
  const minutesRemaining = Math.floor(difference / (1000 * 60));
  return minutesRemaining;
}


function getMinutesUntilMidnight() {
  const now = new Date();

  const midnight = new Date();
  midnight.setDate(now.getDate() + 1);
  midnight.setHours(0, 0, 0, 0); // Set time to 00:00:00

  const difference = midnight - now;
  return Math.floor(difference / (1000 * 60)) + 5; // plus 5 mins to avoid collisions
}

function getFormattedTimestamp(timeZone, format) {
  return Utilities.formatDate(new Date(), timeZone, format);
}

function removeAccentsFromString(str) {
 const accentMap = {
    'á':'a', 'é':'e', 'í':'i', 'ó':'o', 'ö':'o', 'ő':'o', 'ú':'u', 'ü':'u', 'ű':'u',
    'Á':'A', 'É':'E', 'Í':'I', 'Ó':'O', 'Ö':'O', 'Ő':'O', 'Ú':'U', 'Ü':'U', 'Ű':'U',
    'à':'a', 'è':'e', 'ì':'i', 'ò':'o', 'ù':'u', 'ä':'a', 'ë':'e', 'ï':'i', 'ö':'o', 'ü':'u', 'ÿ':'y',
    'À':'A', 'È':'E', 'Ì':'I', 'Ò':'O', 'Ù':'U', 'Ä':'A', 'Ë':'E', 'Ï':'I', 'Ö':'O', 'Ü':'U', 'Ÿ':'Y',
    'á':'a', 'é':'e', 'í':'i', 'ó':'o', 'ú':'u', 'ü':'u',
    'à':'a', 'è':'e', 'ì':'i', 'ò':'o', 'ù':'u',
    'â':'a', 'ê':'e', 'î':'i', 'ô':'o', 'û':'u',
    'ã':'a', 'õ':'o', 'ñ':'n',
    'ç':'c', 'ß':'ss',
    'Á':'A', 'É':'E', 'Í':'I', 'Ó':'O', 'Ú':'U', 'Ü':'U',
    'À':'A', 'È':'E', 'Ì':'I', 'Ò':'O', 'Ù':'U',
    'Â':'A', 'Ê':'E', 'Î':'I', 'Ô':'O', 'Û':'U',
    'Ã':'A', 'Õ':'O', 'Ñ':'N',
    'Ç':'C'
  };

  return str.replace(/[^\u0000-\u007E]/g, function(a) {
    return accentMap[a] || a;
  });
}

function mergeCalendarEvents(calendars, startTime, endTime) {

  let params = { start:startTime, end:endTime, uniqueIds:[] };

  return calendars.map(toUniqueEvents, params)
                  .reduce(toSingleArray)
                  .sort(byStart);
}

function toUniqueEvents(calendar) {
  return calendar.getEvents(this.start, this.end)
                 .filter(onlyUniqueEvents, this.uniqueIds);
}

function onlyUniqueEvents(event) {
  let eventId = event.getId();
  let uniqueEvent = this.indexOf(eventId) < 0;
  if(uniqueEvent) this.push(eventId);
  return uniqueEvent;
}

function toSingleArray(a, b) { return a.concat(b) }

function byStart(a, b) {
  return a.getStartTime().getTime() - b.getStartTime().getTime();
}

// ----- test methods - you can run them directly if you wish -----
function testRemoveAccents() {
  let accentedString = "cliché café résumé ñoño ÁÉÍÓÚÜÀÈÌÒÙÂÊÎÔÛÃÕÇ árvíztűrő tükörfúrógép";
  let withoutAccents = removeAccentsFromString(accentedString);
  Logger.log(withoutAccents);
}

function testMinutesUntilTime() {
  const targetTime = "21:30"; // Example target time
  const minutes = getMinutesUntilTime(targetTime);
  Logger.log(minutes);
}