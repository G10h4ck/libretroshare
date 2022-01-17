#include <cmath>
#include "fsmanager.h"
#include "fsclient.h"

RsFriendServer *rsFriendServer = nullptr;

static const rstime_t MIN_DELAY_BETWEEN_FS_REQUESTS =   30;
static const rstime_t MAX_DELAY_BETWEEN_FS_REQUESTS = 3600;
static const uint32_t DEFAULT_FRIENDS_TO_REQUEST    =   10;

static const std::string DEFAULT_PROXY_ADDRESS         = "127.0.0.1";
static const uint16_t    DEFAULT_FRIEND_SERVER_PORT    = 2017;
static const uint16_t    DEFAULT_PROXY_PORT            = 9050;

FriendServerManager::FriendServerManager()
{
    mLastFriendReqestCampain = 0;
    mFriendsToRequest = DEFAULT_FRIENDS_TO_REQUEST;

    mProxyAddress = DEFAULT_PROXY_ADDRESS;
    mProxyPort = DEFAULT_PROXY_PORT;
    mServerPort = DEFAULT_FRIEND_SERVER_PORT;
}
void FriendServerManager::startServer()
{
    if(!isRunning())
    {
        std::cerr << "Starting Friend Server Manager." << std::endl;
        RsTickingThread::start() ;
    }
}
void FriendServerManager::stopServer()
{
    if(isRunning() && !shouldStop())
    {
        std::cerr << "Stopping Friend Server Manager." << std::endl;
        RsTickingThread::askForStop() ;
    }
}
void FriendServerManager::checkServerAddress_async(const std::string& addr,uint16_t port, uint32_t timeout_ms,
                                                   const std::function<void (const std::string& address,uint16_t port,bool result_status)>& callback)
{
    callback(addr,port,FsClient::checkProxyConnection(addr,port,mProxyAddress,mProxyPort,timeout_ms));
}

void FriendServerManager::setServerAddress(const std::string& addr,uint16_t port)
{
    mServerAddress = addr;
    mServerPort = port;
}
void FriendServerManager::setProxyAddress(const std::string& addr,uint16_t port)
{
    mProxyAddress = addr;
    mProxyPort = port;
}
void FriendServerManager::setFriendsToRequest(uint32_t n)
{
    mFriendsToRequest = n;
}

void FriendServerManager::threadTick()
{
    std::cerr << "Ticking FriendServerManager..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if(mServerAddress.empty())
    {
        RsErr() << "No friend server address has been setup. This is probably a bug.";
        return;
    }
    // Check for requests. Compute how much to wait based on how many friends we have already

    std::vector<RsPgpId> friends;
    rsPeers->getPgpFriendList(friends);

    // log-scale interpolation of the delay between two requests.

    if(mFriendsToRequest == 0 || mFriendsToRequest < friends.size())
    {
        RsErr() << "No friends to request! This is unexpected. Returning." << std::endl;
        return;
    }

    // This formula makes RS wait much longuer between two requests to the server when the number of friends is close the
    // wanted number
    //   Delay for 0 friends: 30 secs.
    //   Delay for 1 friends: 30 secs.
    //   Delay for 2 friends: 32 secs.
    //   Delay for 3 friends: 35 secs.
    //   Delay for 4 friends: 44 secs.
    //   Delay for 5 friends: 66 secs.
    //   Delay for 6 friends: 121 secs.
    //   Delay for 7 friends: 258 secs.
    //   Delay for 8 friends: 603 secs.
    //   Delay for 9 friends: 1466 secs.

    RsDbg() << friends.size() << " friends already, " << mFriendsToRequest << " friends to request";

    double s = (friends.size() < mFriendsToRequest)? ( (mFriendsToRequest - friends.size())/(double)mFriendsToRequest) : 1.0;
    rstime_t delay_for_request = MIN_DELAY_BETWEEN_FS_REQUESTS + (int)floor(exp(-1*s + log(MAX_DELAY_BETWEEN_FS_REQUESTS)*(1.0-s)));

    std::cerr << "Delay for " << friends.size() << " friends: " << delay_for_request << " secs." << std::endl;

    rstime_t now = time(nullptr);

    if(mLastFriendReqestCampain + delay_for_request < now)
    {
        mLastFriendReqestCampain = now;

        std::cerr << "Requesting new friends to friend server..." << std::endl;

        std::map<std::string,bool> friend_certificates;
        FsClient().requestFriends(mServerAddress,mServerPort,mProxyAddress,mProxyPort,mFriendsToRequest,mCachedPGPPassphrase,friend_certificates);	// blocking call

        std::cerr << "Got the following list of friend certificates:" << std::endl;

        // Let's put them in a vector to easy searching.
        std::list<RsPeerId> lst;
        rsPeers->getFriendList(lst);
        std::set<RsPeerId> friend_locations_set(lst.begin(),lst.end());

        for(const auto& invite:friend_certificates)
        {
            RsPeerDetails det;
            uint32_t err_code;

            if(!rsPeers->parseShortInvite(invite.first,det,err_code))
            {
                RsErr() << "Parsing error " << err_code << " in invite \"" << invite.first << "\"";
                continue;
            }

            if(friend_locations_set.find(det.id) != friend_locations_set.end())
            {
                RsDbg() << "    Knw: " << (invite.second?"OK":"--") << " " << det.gpg_id << " " << det.id << " " << det.dyndns;
                continue;
            }

            RsDbg() << "    New: " << (invite.second?"OK":"--") << " " << det.gpg_id << " " << det.id << " " << det.dyndns;

            rsPeers->addSslOnlyFriend(det.id,det.gpg_id,det);
        }
    }
}

