#pragma once

#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/eventreceiver.h"
#include "joblist.h"
#include "path.h"

enum class PrioType;
enum class SitePriority;
class CommandOwner;
class Race;
class TransferJob;
class SiteTransferJob;
class SiteRace;
class FileList;
class File;
class ScoreBoard;
class SiteLogic;
class PendingTransfer;
class Site;
class PreparedRace;
class Path;
class SkipList;
class ScoreBoardElement;
class TransferStatus;

struct QueuedItem {
  enum class Direction { DOWNLOAD, UPLOAD, FXP };
  Direction direction;
  std::string srcSite;
  std::string srcPath;
  std::string srcSection;
  std::string dstSite;
  std::string dstPath;
  std::string dstSection;
  Path localDstPath;
  std::string fileName;
  bool isDirectory;
  unsigned int id;
  unsigned int transferJobId = 0;
  unsigned int getId() const { return id; }

  std::string getDirectionLabel() const {
    switch (direction) {
      case Direction::DOWNLOAD: return "DL";
      case Direction::UPLOAD:   return "UL";
      case Direction::FXP:      return "FXP";
    }
    return "";
  }

  std::string getDisplaySource() const {
    if (direction == Direction::UPLOAD) return "[local]";
    return srcSite;
  }

  std::string getDisplayDest() const {
    if (direction == Direction::DOWNLOAD) return localDstPath.toString();
    return dstSite + ":" + dstPath;
  }

  std::string getRouteKey() const {
    std::string s = getDirectionLabel();
    s += "|" + srcSite + "|" + srcPath + "|" + srcSection;
    s += "|" + dstSite + "|" + dstPath + "|" + dstSection;
    s += "|" + localDstPath.toString();
    return s;
  }

  std::string getDisplayKey() const {
    std::string s = getDirectionLabel();
    s += "|" + srcSite + "|" + dstSite;
    s += "|" + localDstPath.toString();
    return s;
  }
};

struct JobStartResult {
  enum class JobStartState {
    STARTED,
    PREPARED,
    ERROR
  } state;
  JobStartResult();
  JobStartResult(const std::string& error, const std::list<std::string>& infomessages = std::list<std::string>());
  JobStartResult(int id, const std::list<std::string>& infomessages = std::list<std::string>(),
    JobStartResult::JobStartState state = JobStartResult::JobStartState::STARTED);
  operator bool() const;
  unsigned int id;
  std::string error;
  std::list<std::string> infomessages;
};

class Engine : public Core::EventReceiver {
public:
  Engine();
  ~Engine();
  JobStartResult newRace(const std::string& release, const std::string& section, const std::list<std::string>& sites, bool reset = false, const std::list<std::string>& dlonlysites = {});
  JobStartResult prepareRace(const std::string& release, const std::string& section, const std::list<std::string>& sites, bool reset = false, const std::list<std::string>& dlonlysites = {});
  JobStartResult newDistribute(const std::string& release, const std::string& section, const std::list<std::string>& sites, bool reset = false, const std::list<std::string>& dlonlysites = {});
  JobStartResult startPreparedRace(unsigned int);
  void deletePreparedRace(unsigned int);
  JobStartResult startLatestPreparedRace();
  void toggleStartNextPreparedRace();
  JobStartResult newTransferJobDownload(const std::string& srcsite, const std::shared_ptr<FileList>& srcfilelist, const std::string& file, const Path& dstpath);
  JobStartResult newTransferJobDownload(const std::string& srcsite, const std::shared_ptr<FileList>& srcfilelist, const std::string& srcfile, const Path& dstpath, const std::string& dstfile);
  JobStartResult newTransferJobDownload(const std::string& srcsite, const Path& srcpath, const std::string& srcsection, const std::string& srcfile, const Path& dstpath, const std::string& dstfile);
  JobStartResult newTransferJobDownload(const std::string& srcsite, const Path& srcpath, const std::string& srcsection, const std::list<std::string>& files, const Path& dstpath);
  JobStartResult newTransferJobUpload(const Path& srcpath, const std::string& file, const std::string& dstsite, const std::shared_ptr<FileList>& dstfilelist);
  JobStartResult newTransferJobUpload(const Path& srcpath, const std::string& srcfile, const std::string& dstsite, const std::shared_ptr<FileList>& dstfilelist, const std::string& dstfile);
  JobStartResult newTransferJobUpload(const Path& srcpath, const std::string& srcfile, const std::string& dstsite, const Path& dstpath, const std::string& dstsection, const std::string& dstfile);
  JobStartResult newTransferJobUpload(const Path& srcpath, const std::list<std::string>& files, const std::string& dstsite, const Path& dstpath, const std::string& dstsection);
  JobStartResult newTransferJobFXP(const std::string& srcsite, const std::shared_ptr<FileList>& srcfilelist, const std::string& dstsite, const std::shared_ptr<FileList>& dstfilelist, const std::string& file);
  JobStartResult newTransferJobFXP(const std::string& srcsite, const std::shared_ptr<FileList>& srcfilelist, const std::string& srcfile, const std::string& dstsite, const std::shared_ptr<FileList>& dstfilelist, const std::string& dstfile);
  JobStartResult newTransferJobFXP(const std::string& srcsite, const Path& srcpath, const std::string& srcsection, const std::string& srcfile, const std::string& dstsite, const Path& dstpath, const std::string& dstsection, const std::string& dstfile);
  JobStartResult newTransferJobFXP(const std::string& srcsite, const Path& srcpath, const std::string& srcsection, const std::list<std::string>& files, const std::string& dstsite, const Path& dstpath, const std::string& dstsection);
  void removeSiteFromRace(const std::shared_ptr<Race> &, const std::string &);
  void removeSiteFromAllRunningSpreadJobs(const std::string& site);
  void removeSiteFromRaceDeleteFiles(const std::shared_ptr<Race>& race, const std::string& site, bool allfiles, bool deleteoncomplete);
  void abortRace(const std::shared_ptr<Race>& race);
  void resetRace(const std::shared_ptr<Race>& race, bool hard);
  void deleteOnAllSites(const std::shared_ptr<Race>& race, bool allfiles = false, bool deleteoncomplete = false);
  void deleteOnSites(const std::shared_ptr<Race>& race, std::list<std::shared_ptr<Site>>, bool allfiles = false);
  void abortTransferJob(const std::shared_ptr<TransferJob>& tj);
  void resetTransferJob(const std::shared_ptr<TransferJob>& tj);
  void jobFileListRefreshed(SiteLogic *, const std::shared_ptr<CommandOwner> & commandowner, const std::shared_ptr<FileList>& fl);
  bool transferJobActionRequest(const std::shared_ptr<SiteTransferJob> &);
  void raceActionRequest();
  void setPreparedRaceExpiryTime(int expirytime);
  void setNextPreparedRaceStarterTimeout(int timeout);
  unsigned int preparedRaces() const;
  unsigned int currentRaces() const;
  unsigned int allRaces() const;
  unsigned int currentTransferJobs() const;
  unsigned int allTransferJobs() const;
  std::shared_ptr<Race> getRace(unsigned int id) const;
  std::shared_ptr<Race> getRace(const std::string& race) const;
  std::shared_ptr<TransferJob> getTransferJob(unsigned int id) const;
  std::shared_ptr<TransferJob> getTransferJob(const std::string& tj) const;
  std::list<std::shared_ptr<PreparedRace>>::const_iterator getPreparedRacesBegin() const;
  std::list<std::shared_ptr<PreparedRace>>::const_iterator getPreparedRacesEnd() const;
  std::list<std::shared_ptr<Race>>::const_iterator getRacesBegin() const;
  std::list<std::shared_ptr<Race>>::const_iterator getRacesEnd() const;
  std::list<std::shared_ptr<Race>>::const_iterator getCurrentRacesBegin() const;
  std::list<std::shared_ptr<Race>>::const_iterator getCurrentRacesEnd() const;
  std::list<std::shared_ptr<Race>>::const_iterator getFinishedRacesBegin() const;
  std::list<std::shared_ptr<Race>>::const_iterator getFinishedRacesEnd() const;
  std::list<std::shared_ptr<TransferJob>>::const_iterator getCurrentTransferJobsBegin() const;
  std::list<std::shared_ptr<TransferJob>>::const_iterator getCurrentTransferJobsEnd() const;
  std::list<std::shared_ptr<TransferJob>>::const_iterator getTransferJobsBegin() const;
  std::list<std::shared_ptr<TransferJob>>::const_iterator getTransferJobsEnd() const;
  void tick(int);
  void addSiteToRace(const std::shared_ptr<Race>& race, const std::string& site, bool downloadonly);
  std::shared_ptr<ScoreBoard> getScoreBoard() const;
  int getMaxPointsRaceTotal() const;
  int getMaxPointsFileSize() const;
  int getMaxPointsAvgSpeed() const;
  int getMaxPointsPriority() const;
  int getMaxPointsPercentageOwned() const;
  int getMaxPointsLowProgress() const;
  int getPriorityPoints(SitePriority priority) const;
  int getSpeedPoints(int) const;
  int getPreparedRaceExpiryTime() const;
  bool getNextPreparedRaceStarterEnabled() const;
  int getNextPreparedRaceStarterTimeout() const;
  int getNextPreparedRaceStarterTimeRemaining() const;
  bool isIncompleteEnoughForDelete(const std::shared_ptr<Race> & race, const std::shared_ptr<SiteRace> & siterace) const;
  void transferFailed(const std::shared_ptr<TransferStatus> & ts, int err);
  unsigned int addToQueue(const std::shared_ptr<QueuedItem>& item);
  bool isInQueue(const QueuedItem& item) const;
  JobStartResult startQueuedItem(const std::shared_ptr<QueuedItem>& item);
  JobStartResult startQueueBatch(const std::shared_ptr<QueuedItem>& item);
  JobStartResult startAllQueuedBatches();
  unsigned int countStartedQueueItems() const;
  void removeFromQueue(unsigned int id);
  void clearQueue();
  size_t getQueueSize() const;
  std::list<std::shared_ptr<QueuedItem>>::const_iterator getQueueBegin() const;
  std::list<std::shared_ptr<QueuedItem>>::const_iterator getQueueEnd() const;
  std::shared_ptr<QueuedItem> getQueuedItemById(unsigned int id) const;
  bool moveQueueItemUp(unsigned int id);
  bool moveQueueItemDown(unsigned int id);
  void stopTransferJobAfterRelease(unsigned int queueId);
  void stopTransferJobAfterFile(unsigned int queueId);
  void stopTopOfQueue(bool stopAfterRelease);
  int getMaxSpreadJobsHistory() const;
  int getMaxTransferJobsHistory() const;
  void setMaxSpreadJobsHistory(int jobs);
  void setMaxTransferJobsHistory(int jobs);
  int getMaxSpreadJobTimeSeconds() const;
  void setMaxSpreadJobTimeSeconds(int seconds);
 private:
  JobStartResult newSpreadJob(int profile, const std::string& release, const std::string& section, const std::list<std::string>& sites, bool reset, const std::list<std::string>& dlonlysites);
  void estimateRaceSizes();
  void estimateRaceSize(const std::shared_ptr<Race>&, bool forceupdate = false);
  void reportCurrentSize(const SkipList&, const SkipList&, const std::shared_ptr<SiteRace>& srs, const std::shared_ptr<FileList>& fl, bool final);
  void addToScoreBoard(const std::shared_ptr<FileList>& fl, const std::shared_ptr<SiteRace>& sr, const std::shared_ptr<SiteLogic>& sl);
  void addToScoreBoardForPair(const std::shared_ptr<SiteLogic>& sls,
      const std::shared_ptr<Site> & ss, const std::shared_ptr<SiteRace>& srs,
      const std::shared_ptr<FileList>& fls, const std::shared_ptr<SiteLogic>& sld, const std::shared_ptr<Site>& ds,
      const std::shared_ptr<SiteRace>& srd, const std::shared_ptr<FileList>& fld, const SkipList& dstskip,
      const SkipList& secskip,
      const std::shared_ptr<Race>& race, const Path& subpath, SitePriority priority,
      bool racemode);
  void updateScoreBoard();
  void refreshScoreBoard();
  void issueOptimalTransfers();
  void setSpeedScale();
  unsigned short calculateScore(PrioType priotype, unsigned long long int filesize, const std::shared_ptr<Race>&, const std::shared_ptr<FileList>& fls, const std::shared_ptr<SiteRace>& srs, const std::shared_ptr<FileList>& fld, const std::shared_ptr<SiteRace> & srd, int, SitePriority priority, bool) const;
  unsigned short calculateScore(ScoreBoardElement* sbe) const;
  void checkIfRaceComplete(const std::shared_ptr<SiteLogic>& sls, std::shared_ptr<Race>& race);
  void raceComplete(const std::shared_ptr<Race>& race);
  void issueGlobalComplete(const std::shared_ptr<Race>& race);
  void refreshPendingTransferList(const std::shared_ptr<TransferJob>& tj);
  void checkStartPoke();
  void addPendingTransfer(std::list<PendingTransfer>&, PendingTransfer&);
  std::shared_ptr<Race> getCurrentRace(const std::string &) const;
  void preSeedPotentialData(const std::shared_ptr<Race>& race);
  void wipeFromScoreBoard(const std::shared_ptr<SiteRace>& sr);
  bool waitingInScoreBoard(const std::shared_ptr<Race>& race) const;
  void restoreFromFailed(const std::shared_ptr<Race>& race);
  void removeFromFinished(const std::shared_ptr<Race>& race);
  void clearSkipListCaches();
  void rotateSpreadJobsHistory();
  void rotateTransferJobsHistory();
  JobList<std::shared_ptr<QueuedItem>> transferqueue;
  JobList<std::shared_ptr<Race>> allraces;
  JobList<std::shared_ptr<Race>> currentraces;
  JobList<std::shared_ptr<Race>> finishedraces;
  JobList<std::shared_ptr<PreparedRace>> preparedraces;
  JobList<std::shared_ptr<TransferJob>> alltransferjobs;
  JobList<std::shared_ptr<TransferJob>> currenttransferjobs;
  std::shared_ptr<ScoreBoard> scoreboard;
  std::shared_ptr<ScoreBoard> failboard;
  std::unordered_map<std::shared_ptr<TransferJob>, std::list<PendingTransfer>> pendingtransfers;
  int maxavgspeed;
  bool pokeregistered;
  unsigned int dropped;
  unsigned int nextqueueid;
  unsigned int nextid;
  int maxpointsfilesize;
  int maxpointsavgspeed;
  int maxpointspriority;
  int maxpointspercentageowned;
  int maxpointslowprogress;
  int preparedraceexpirytime;
  int startnextpreparedtimeout;
  bool startnextprepared;
  int nextpreparedtimeremaining;
  std::unordered_map<std::shared_ptr<FileList>, std::pair<std::shared_ptr<SiteRace>, std::shared_ptr<SiteLogic>>> spreadjobfilelistschanged;
  bool forcescoreboard;
  std::unordered_set<std::shared_ptr<Site>> skiplistcachesites;
  std::unordered_set<std::string> skiplistcachesections;
  int maxspreadjobshistory;
  int maxtransferjobshistory;
  int maxspreadjobtimeseconds;
};
