#include "DeleteFileDetection.h"
#include "XMLInfo.h"
#include "TaskQueue.h"
#include "Configure.h"

#ifdef WIN32
#include <Windows.h>
#endif
#include <glog/logging.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

DeleteFileDetection::DeleteFileDetection(void)
{
}


DeleteFileDetection::~DeleteFileDetection(void)
{
}

void DeleteFileDetection::run(void)
{
	LOG(INFO) << "Delete File detection begin ." ;

    while(true)
    {
		// ɨ������HLS��Ŀ¼�����Ƿ�ʱ�䳬����5�죬��������˾Ͱ��ļ�����Ŀ¼ɾ����
		if(boost::filesystem::exists(Configure::instance().m_outputTSPath.objectName) + "\\")
        {
			boost::filesystem::directory_iterator itBegin(Configure::instance().m_outputTSPath.objectName+ "\\" );
            boost::filesystem::directory_iterator itEnd;
            for(;itBegin != itEnd; itBegin++)
            {
                if(itBegin->path().string().find(Configure::instance().m_encodeChannelPrefix) == std::string::npos)
                {
                    continue;
                }

				std::time_t t = boost::filesystem::last_write_time(itBegin->path()); 
				double diff = difftime(time(NULL), t);
				// ����DELETEFILE_DATE�죬ɾ����
				if(diff/3600/24 > Configure::instance().m_keepDateTime)
				{
					LOG(INFO)  << "Find : " << itBegin->path().string() << " create in " << std::ctime(&t)<< " is over " \
                        << Configure::instance().m_keepDateTime <<  " date. remove it" ;
					try
					{
						boost::filesystem::remove_all(itBegin->path());
					}
					catch(...)
					{
						LOG(INFO) << "Can not remove file " << itBegin->path().string();
					}
				}
			}
		}
		else
		{
			LOG(INFO) << "replaytv path [" << Configure::instance().m_outputTSPath.path << "] is not exist." ;
		}

		// ɨ������xml��Ŀ¼�����Ƿ�ʱ�䳬����5�죬����ǾͰ��ļ�����Ŀ¼ɾ����
		if(boost::filesystem::exists(Configure::instance().m_outputXMLPath.objectName) + "\\")
        {
			boost::filesystem::directory_iterator itBegin(Configure::instance().m_outputXMLPath.objectName+ "\\" );
            boost::filesystem::directory_iterator itEnd;
            for(;itBegin != itEnd; itBegin++)
            {
                if(itBegin->path().string().find(Configure::instance().m_encodeChannelPrefix) == std::string::npos)
                {
                    continue;
                }

				std::time_t t = boost::filesystem::last_write_time(itBegin->path()); 
				double diff = difftime(time(NULL), t);
				// ����DELETEFILE_DATE�죬ɾ����
                if(diff/3600/24 > Configure::instance().m_keepDateTime)
				{
					LOG(INFO)  << "Find : " << itBegin->path().string() << " create in " << std::ctime(&t)<< " is over " \
                        << Configure::instance().m_keepDateTime <<  " date. remove it" ;
					try
					{
						boost::filesystem::remove_all(itBegin->path());
					}
					catch(...)
					{
						LOG(INFO) << "Can not remove file " << itBegin->path().string();
					}
				}
			}
		}
		else
		{
			LOG(INFO) << "rtvProcessed path [" << Configure::instance().m_outputXMLPath.path << "] is not exist." ;
		}

#ifdef WIN32
        // ÿ��ָ��ʱ�䣬��ɨ��һ��Ŀ¼
        Sleep(15 * 60 * 1000);
#else
#endif
    }
}

void DeleteFileDetection::start(void)
{
    m_thread.reset(new boost::thread( boost::bind(&DeleteFileDetection::run,this)));
}
