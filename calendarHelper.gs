const daysToViewAhead = 1;

function doGet(e) {

  var calendars = CalendarApp.getAllOwnedCalendars();
  
  if (calendars == undefined) {
    const error = "Can not access any calendar";
    Logger.log(error);
    return ContentService.createTextOutput({ "error": error }).setMimeType(ContentService.MimeType.JSON);
  }

  var start = new Date(); 
  start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + daysToViewAhead * oneday);
  var events = mergeCalendarEvents(calendars, start, stop); //pull start/stop time

  // exclude the INVITED and NO answered events
  events = events.filter(event =>
    event.getMyStatus() == CalendarApp.GuestStatus.OWNER ||
    event.getMyStatus() == CalendarApp.GuestStatus.YES ||
    event.getMyStatus() == CalendarApp.GuestStatus.MAYBE
  );

  eventArray = [];
  for (var i = 0; i < events.length; i++) {
    eventArray.push({ 
      "name": removeAccentsFromString(events[i].getTitle()),
      "allDayEvent" : events[i].isAllDayEvent(),
      "startDate": Utilities.formatDate(events[i].getStartTime(), 'Etc/GMT', 'yyyy-MM-dd\'T\'HH:mm\'Z\''),
      "endDate": Utilities.formatDate(events[i].getEndTime(), 'Etc/GMT', 'yyyy-MM-dd\'T\'HH:mm\'Z\'')
    });
  }

  const respoonse = { "events": eventArray };
  Logger.log(JSON.stringify(respoonse));
  
  return ContentService.createTextOutput(JSON.stringify(respoonse)).setMimeType(ContentService.MimeType.JSON);
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

// TODO test method
function testRemoveAccents() {
  var accentedString = "cliché café résumé ñoño ÁÉÍÓÚÜÀÈÌÒÙÂÊÎÔÛÃÕÇ árvíztűrő tükörfúrógép";
  var withoutAccents = removeAccentsFromString(accentedString);
  Logger.log(withoutAccents);
}

function mergeCalendarEvents(calendars, startTime, endTime) {

  var params = { start:startTime, end:endTime, uniqueIds:[] };

  return calendars.map(toUniqueEvents, params)
                  .reduce(toSingleArray)
                  .sort(byStart);
}

function toUniqueEvents(calendar) {
  return calendar.getEvents(this.start, this.end)
                 .filter(onlyUniqueEvents, this.uniqueIds);
}

function onlyUniqueEvents(event) {
  var eventId = event.getId();
  var uniqueEvent = this.indexOf(eventId) < 0;
  if(uniqueEvent) this.push(eventId);
  return uniqueEvent;
}

function toSingleArray(a, b) { return a.concat(b) }

function byStart(a, b) {
  return a.getStartTime().getTime() - b.getStartTime().getTime();
}
