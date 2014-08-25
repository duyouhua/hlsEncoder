#include "EncoderManager.h"
#include "MailClient.h"
#include <iostream>

#ifdef WIN32
#include <Windows.h>
#endif

#include <kugou/base/facility/log.h>


EncoderManager::EncoderManager(void)
{
}


EncoderManager::~EncoderManager(void)
{
}

void EncoderManager::update(const std::string& address)
{
    boost::mutex::scoped_lock scopedLock(m_mutexEncoderMap);

    // �����������������Ƿ��Ѿ�����
    std::map<std::string, Poco::Timestamp>::iterator it = m_encoderMap.find(address);
    if(it == m_encoderMap.end())
    {
        // ��һ��ע�����
        Poco::Timestamp t;
        m_encoderMap.insert(std::pair<std::string, Poco::Timestamp>(address,t));
		std::string content = "<Info> : Encoder [" + address + "] Start.)";
        LOG_I(content);

		MailClient mail;
		mail.Send(content);
    }
    else
    {
        // �Ѿ�����
        it->second.update();
        LOG_I("Encoder client [" << address << " heartbeat.)");
    }
}

void EncoderManager::run(void)
{
    while(1)
    {
        {
            boost::mutex::scoped_lock scopedLock(m_mutexEncoderMap);
            EncoderMap::iterator it = m_encoderMap.begin();
            while(it != m_encoderMap.end())
            {
                // 2����û�����������ı�������Ҫɾ����
                //std::cout << it->second.elapsed() << std::endl;
                if(it->second.elapsed() > 120000000)   // 120�� // elapsed ��λ��1��=1000000
                {
                    //std::cout << "delete it : " << it->second.elapsed() << std::endl;
					std::string content = "<Error> : Encoder [" + it->first + "] Stop.";
                    LOG_W(content);
                    m_encoderMap.erase(it++);

					MailClient mail;
					mail.Send(content);
                }
                else
                {
                    ++it;
                }
            }  
            
            //MailClient client;
            //client.Send();
        }
        
        // ÿ��10����һ��
        Sleep(10 * 1000);
    }
}

void EncoderManager::start(void)
{
    m_thread.reset(new boost::thread( boost::bind(&EncoderManager::run,this)));
}

