#include "FileDetection.h"
#include "XMLInfo.h"
#include "TaskQueue.h"
#include "Configure.h"

#ifdef WIN32
#include <Windows.h>
#endif
#include <glog/logging.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"

FileDetection::FileDetection(std::string path)
    : m_scanPath(path)
    , m_allFileSet(new StringSet)
{
    // �ж�Ҫɨ���Ŀ¼�Ƿ����
    if(!boost::filesystem::exists(m_scanPath))
    {
        LOG(ERROR) << "scan path [" << m_scanPath << "] is not exist." ;
        throw std::string("scan path is not exist.");
    }
}


FileDetection::~FileDetection(void)
{
}

void FileDetection::run(void)
{
	LOG(INFO) << "File detection begin ." ;
    m_lastXMLGeneraateTime.update();

    while(true)
    {
        boost::shared_ptr<StringSet>  newSet(new StringSet);

        //����4Сʱ��û�з����µ�xml�ļ�����Ҫ�ϱ������
        if(m_lastXMLGeneraateTime.elapsed() > 14400000000)   // 4*3600�� // elapsed ��λ��1��=1000000
        {
            LOG(WARNING) << "No new xml file in 4 hour" ;
            // ���͸����
            try
            {
                std::string host = "10.27.69.84";
                int         port = 9090;
                LOG(INFO) << "Send Message to " << host << ":" << port;
                Poco::Net::HTTPClientSession s(host.c_str(), port);
	            Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, "/api/monitor/message.json");
	            std::string body("{\"Message\":\"Channel[" + Configure::instance().m_encodeChannel + "] No new xml file in 4 hour\"}");
            
	            request.setContentLength((int) body.length());
	            s.sendRequest(request) << body;
	            Poco::Net::HTTPResponse response;
	            std::istream& rs = s.receiveResponse(response);
	            std::ostringstream ostr;
	            Poco::StreamCopier::copyStream(rs, ostr);
                m_lastXMLGeneraateTime.update();
            }
            catch(...)
            {
                LOG(INFO) << "Send Message exception.";
            }
        }

        if(boost::filesystem::exists(m_scanPath))
        {
            boost::filesystem::directory_iterator itBegin(m_scanPath);
            boost::filesystem::directory_iterator itEnd;

            for(;itBegin != itEnd; itBegin++)
            {
                try
                {
                    if (boost::filesystem::is_directory(*itBegin))
                    {
                        // ��ʱ��֧����Ŀ¼ɨ��
                        //boost::filesystem::path p = itBegin->path();
                        //PrintAllFile(p);
                    }
                    else
                    {
                        //�������ļ��ǲ����Ѿ�ɨ���
                        std::string fileName(itBegin->path().string());
                        if(this->m_allFileSet->find(fileName) == m_allFileSet->end())
                        {
                            // �·����ļ����ȼ����ļ��Ƿ�������Ҫ�ҵ�xml�ļ�
                            XMLInfo xmlInfo;
                            if(xmlInfo.load(itBegin->path().string()))
                            {
                                if(xmlInfo.AssetName.empty())
                                {
                                    LOG(INFO)  << "Find : " << itBegin->path().string() << ".  but its TS file is empty" ;
                                }
                                else
                                {
                                    LOG(INFO)  << "Find : " << itBegin->path().string() << ".  its TS file : " <<
                                        xmlInfo.AssetName ;
                                    //���TS�ļ��Ƿ���ڣ���������ڣ���֪ͨ���
                                    if(!boost::filesystem::exists(std::string(Configure::instance().m_inputTSPath.objectName + \
                                        "\\" + xmlInfo.AssetName).c_str()))
                                    {
                                        LOG(WARNING) << "Can not find TS file : " << std::string(Configure::instance().m_inputTSPath.objectName + "\\" + xmlInfo.AssetName).c_str();
                                        // ���͸����
                                        try
                                        {
                                            std::string host = "10.27.69.84";
                                            int         port = 9090;
                                            LOG(INFO) << "Send Message to " << host << ":" << port;
                                            Poco::Net::HTTPClientSession s(host.c_str(), port);
	                                        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, "/api/monitor/message.json");
	                                        std::string body("{\"Message\":\"Can not find TS file ["+ xmlInfo.AssetName + "]\"}");
            
	                                        request.setContentLength((int) body.length());
	                                        s.sendRequest(request) << body;
	                                        Poco::Net::HTTPResponse response;
	                                        std::istream& rs = s.receiveResponse(response);
	                                        std::ostringstream ostr;
	                                        Poco::StreamCopier::copyStream(rs, ostr);
                                        }
                                        catch(...)
                                        {
                                            LOG(INFO) << "Send Message exception.";
                                        }
                                    }
									else
									{
										TaskQueue::instance().producer(itBegin->path().string(), xmlInfo.AssetName);
									}

                                    m_lastXMLGeneraateTime.update();
                                }
                            }
                        }
                        newSet->insert(fileName);
                    }
                }
                catch (const std::exception & ex )
                {
                    LOG(ERROR)<< ex.what() ;
                    continue;
                }
            }

            // �����б�Ϊ��һ��ɨ����б�
            this->m_allFileSet = newSet;
        }
        else
        {
            LOG(INFO) << "scan path [" << m_scanPath << "] is not exist." ;
        }

#ifdef WIN32
        // ÿ��ָ��ʱ�䣬��ɨ��һ��Ŀ¼
        Sleep(SCAN_INTERVAL * 1000);
#else
#endif
    }
}

void FileDetection::start(void)
{
    m_thread.reset(new boost::thread( boost::bind(&FileDetection::run,this)));
}
