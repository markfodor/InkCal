// E.g: 'Europe/Madrid' -> you can override this if you want to differ from your Google account setup
// https://developers.google.com/google-ads/api/data/codes-formats#timezone-ids
const timeZone = Session.getScriptTimeZone();
const daysToViewAhead = 1;    // The days ahead you wish to dispay, ideally it is only the current day so 1
const dateFormatPattern = 'yyyy.MM.dd';
const timePattern = 'HH:mm';
const maxAllDayEventNumber = 8;
const maxShortEventNumber = 5;

function doGet(e) {
  try {
    return buildResponseJson();
  } catch (e) {
    Logger.log(e.stack);

    const message = {"error": e.toString()}
    const jsonString = JSON.stringify(message);
    Logger.log(jsonString); // uncomment this line for testing
    return ContentService.createTextOutput(jsonString).setMimeType(ContentService.MimeType.JSON);
  }
}

function buildResponseJson() {
    const calendars = CalendarApp.getAllOwnedCalendars();

    if (calendars == undefined) {
      const error = "Can not access any calendar";
      Logger.log(error);
      return ContentService.createTextOutput({ "error": error }).setMimeType(ContentService.MimeType.JSON);
    }

    let fromDate = new Date(); 
    fromDate.setHours(0, 0, 0);  // start at midnight
    const oneday = 24*3600000; // [msec]
    const toDate = new Date(fromDate.getTime() + daysToViewAhead * oneday);
    let events = mergeCalendarEvents(calendars, fromDate, toDate);

    // exclude the INVITED and NO answered events --> include only your own events and the ones you answared YES or MAYBE
    events = events.filter(event =>
      event.getMyStatus() == CalendarApp.GuestStatus.OWNER ||
      event.getMyStatus() == CalendarApp.GuestStatus.YES ||
      event.getMyStatus() == CalendarApp.GuestStatus.MAYBE
    );

    eventArray = [];
    allDayEventCounter = 0;
    shortEventCounter = 0;
    for (let i = 0; i < events.length; i++) {
      if (events[i].isAllDayEvent() && allDayEventCounter < maxAllDayEventNumber) {
        eventArray.push(buildEventObject(events[i]));
        allDayEventCounter++;
      }

      // only add the relevant events -> the ones where the end time is bigger than the current date
      // so if the boards requests an update in the middle of an event, it will be displayed
      if (!events[i].isAllDayEvent() && shortEventCounter < maxShortEventNumber && events[i].getEndTime() > new Date()) {
        eventArray.push(buildEventObject(events[i]));
        shortEventCounter++;
      } 
    }

    const respoonse = {
      "date": getFormattedTimestamp(timeZone, dateFormatPattern),
      "time": getFormattedTimestamp(timeZone, timePattern),
      "sleep": calculateDeepSleepInMins(events),
      "events": eventArray
    };
    const jsonString = JSON.stringify(respoonse); 

    Logger.log(jsonString); // uncomment this line for testing
    return ContentService.createTextOutput(jsonString).setMimeType(ContentService.MimeType.JSON);
}

function buildEventObject(event) {
  return { 
        "name": removeAccentsFromString(event.getTitle()),
        "allDayEvent" : event.isAllDayEvent(),
        "startTime": Utilities.formatDate(event.getStartTime(), timeZone, timePattern), // TODO rename these to startTime, endTime
        "endTime": Utilities.formatDate(event.getEndTime(), timeZone, timePattern)
      }
}

function calculateDeepSleepInMins(events) {
  const now = new Date();  // used for every calculation to eliminate elapsed time between calculations
  const minsUntilMidnight = getMinutesUntilMidnight(now);
  let minsUntilNextEvent = 99999;
  
  for (let i = 0; i < events.length; i++) {
    if (!events[i].allDayEvent) {
      const minsUntilNext = getMinutesUntil(events[i].getEndTime(), now);
      if (minsUntilNext < minsUntilNextEvent) {
        minsUntilNextEvent = minsUntilNext;
      }
    }
  }

  const sleepInMins = (minsUntilMidnight < minsUntilNextEvent) ? minsUntilMidnight : minsUntilNextEvent;
  // ESP timer is not precise if it needs to be in deep sleep for longer periods (especially on lower battery levels), it may wake up earlier
  // 5 min plus eliminates the number of updates if ESP needs to refresh the screen when a short event has ended
  return sleepInMins + 5;  
}

function getMinutesUntil(endDate, now) {
  // If the target time is earlier than the current time
  if (endDate < now) {
    return 99999;
  }

  // Calculate the difference in milliseconds and convert to minutes
  const difference = endDate - now;
  const minutesRemaining = Math.floor(difference / (1000 * 60));
  return minutesRemaining;
}

function getMinutesUntilMidnight(now) {
  const midnight = new Date();
  midnight.setDate(now.getDate() + 1);
  midnight.setHours(0, 0, 0, 0); // Set time to 00:00:00

  const difference = midnight - now;
  return Math.floor(difference / (1000 * 60));
}

// used with date and time patterns as well
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
  const targetTime = new Date();
  targetTime.setHours(24, 0, 0, 0);

  const now = new Date();
  now.setHours(23, 0, 0, 0);

  const minutes = getMinutesUntil(targetTime, now);
  Logger.log(minutes);
}