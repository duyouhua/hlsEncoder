#include "EncoderManager.h"
#include "MailClient.h"
#include <iostream>

#ifdef WIN32
#include <Windows.h>
#endif

#include <ocn/base/facility/log.h>


EncoderManager::EncoderManager(void)
{
}


EncoderManager::~EncoderManager(void)
{
}

void EncoderManager::message(const std::string& address, std::string& mess)
{
    LOG_I("Send message : " + mess);
	MailClient mail;
	mail.Send(mess);
}

void EncoderManager::update(const std::string& address)
{
    boost::mutex::scoped_lock scopedLock(m_mutexEncoderMap);

    // 查找这个编码服务器是否已经存在
    std::map<std::string, Poco::Timestamp>::iterator it = m_encoderMap.find(address);
    if(it == m_encoderMap.end())
    {
        // 第一次注册进来
        Poco::Timestamp t;
        m_encoderMap.insert(std::pair<std::string, Poco::Timestamp>(address,t));
		std::string content = "<Info> : Encoder [" + address + "] Start.";
        LOG_I(content);

		MailClient mail;
		mail.Send(content);

        //保存文件
        ActiveEncoder ae;
        if(!ae.fromJsonFile((m_exePath + "\\data\\ActiveEncoder.txt").c_str()))
        {
            LOG_E("open ActiveEncoder error");
            return;
        }
        ae.EncoderAddress.push_back(address);
        ae.toJson((m_exePath + "\\data\\ActiveEncoder.txt").c_str());
    }
    else
    {
        // 已经存在
        it->second.update();
        LOG_I("Encoder client [" << address << " heartbeat.)");
    }
}

void EncoderManager::setExePath(std::string& path)
{
    m_exePath = path;
}

void EncoderManager::run(void)
{
    // 重新把启动前已经上报到监控的编码器load回来
    ActiveEncoder ae;
    if(!ae.fromJsonFile((m_exePath + "\\data\\ActiveEncoder.txt").c_str()))
    {
        LOG_E("open ActiveEncoder error");
        return;
    }
    for(std::vector<std::string>::iterator it = ae.EncoderAddress.begin(); it != ae.EncoderAddress.end(); it++)
    {
        Poco::Timestamp t;
        m_encoderMap.insert(std::pair<std::string, Poco::Timestamp>(*it,t));
    }

    //
    while(1)
    {
        {
            boost::mutex::scoped_lock scopedLock(m_mutexEncoderMap);
            EncoderMap::iterator it = m_encoderMap.begin();
            while(it != m_encoderMap.end())
            {
                // 2分钟没有连接上来的编码器，要删除掉
                //std::cout << it->second.elapsed() << std::endl;
                if(it->second.elapsed() > 120000000)   // 120秒 // elapsed 单位是1秒=1000000
                {
                    //std::cout << "delete it : " << it->second.elapsed() << std::endl;
					std::string content = "<Error> : Encoder [" + it->first + "] Stop.";
                    std::string address = it->first;
                    LOG_W(content);
                    m_encoderMap.erase(it++);

					MailClient mail;
					mail.Send(content);

                    //保存文件
                    ActiveEncoder ae;
                    if(!ae.fromJsonFile((m_exePath + "\\data\\ActiveEncoder.txt").c_str()))
                    {
                        LOG_E("open ActiveEncoder error");
                        return;
                    }
                    for(std::vector<std::string>::iterator Eit = ae.EncoderAddress.begin(); Eit != ae.EncoderAddress.end(); Eit++)
                    {
                        if((*Eit).compare(address) == 0)
                        {
                            Eit = ae.EncoderAddress.erase(Eit);
                            break;
                        }
                    }
                    ae.toJson((m_exePath + "\\data\\ActiveEncoder.txt").c_str());
                }
                else
                {
                    ++it;
                }
            }  
            
            //MailClient client;
            //client.Send();
        }
        
        // 每隔10秒检测一次
        Sleep(10 * 1000);
    }
}

void EncoderManager::start(void)
{
    m_thread.reset(new boost::thread( boost::bind(&EncoderManager::run,this)));
}

