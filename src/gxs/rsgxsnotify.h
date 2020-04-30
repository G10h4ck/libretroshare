/*******************************************************************************
 * libretroshare/src/gxs/: rsgxsnotify.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2015  Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

/*!
 * The aim of this class is to implement notifications internally to GXS, which are
 * mostly used by RsGenExchange to send information to specific services. These services
 * then interpret these changes and turn them into human-readable/processed service-specific changes.
 */

struct RsGxsNotify
{
	enum NotifyType
	{
        TYPE_UNKNOWN                           = 0x00,
        TYPE_PUBLISHED                         = 0x01,
        TYPE_RECEIVED_NEW                      = 0x02,
        TYPE_PROCESSED                         = 0x03,
        TYPE_RECEIVED_PUBLISHKEY               = 0x04,
        TYPE_RECEIVED_DISTANT_SEARCH_RESULTS   = 0x05,
        TYPE_STATISTICS_CHANGED                = 0x06,
        TYPE_UPDATED                           = 0x07,
        TYPE_MESSAGES_DELETED                  = 0x08,
        TYPE_GROUP_DELETED                     = 0x09,
	};

	virtual ~RsGxsNotify() {}
	virtual NotifyType getType() = 0;
};

/*!
 * Relevant to group changes
 */
class RsGxsGroupChange : public RsGxsNotify
{
public:
	RsGxsGroupChange(NotifyType type, bool metaChange) : mNewGroupItem(nullptr),mOldGroupItem(nullptr), mNotifyType(type), mMetaChange(metaChange) {}
    virtual ~RsGxsGroupChange() { delete mOldGroupItem; delete mNewGroupItem ; }

    NotifyType getType() override { return mNotifyType;}
    bool metaChange() { return mMetaChange; }

    RsGxsGroupId mGroupId;			// Group id of the group we're talking about. When the group is deleted, it's useful to know which group
    								// that was although there is no pointers to the actual group data anymore.
    RsGxsGrpItem *mNewGroupItem;	// Valid when a group has changed, or a new group is received.
    RsGxsGrpItem *mOldGroupItem;	// only valid when mNotifyType is TYPE_UPDATED

protected:
    NotifyType mNotifyType;
    bool mMetaChange;
};

class RsGxsDistantSearchResultChange: public RsGxsNotify
{
public:
    RsGxsDistantSearchResultChange(TurtleRequestId id,const RsGxsGroupId& group_id) : mRequestId(id),mGroupId(group_id){}

    NotifyType getType() { return TYPE_RECEIVED_DISTANT_SEARCH_RESULTS ; }

    TurtleRequestId mRequestId ;
 	RsGxsGroupId mGroupId;
};

/*!
 * Relevant to message changes
 */
class RsGxsMsgChange : public RsGxsNotify
{
public:
	RsGxsMsgChange(NotifyType type, bool metaChange) : NOTIFY_TYPE(type), mMetaChange(metaChange) {}
    std::map<RsGxsGroupId, std::set<RsGxsMessageId> > msgChangeMap;
	NotifyType getType(){ return NOTIFY_TYPE;}
    bool metaChange() { return mMetaChange; }
private:
    const NotifyType NOTIFY_TYPE;
    bool mMetaChange;
};

