#include "myhttprequest.h"
#include "utility.h"
#include "mynetworkaccessmanagerfactory.h"

MyHttpRequest::MyHttpRequest(QObject *parent) :
    QObject(parent)
{
    m_status = Idle;
    manager = new NetworkAccessManager(this);
    connect (manager, SIGNAL(finished(QNetworkReply*)), SLOT(finished(QNetworkReply*)));
}

MyHttpRequest::RequestStatus MyHttpRequest::status()
{
    return m_status;
}

void MyHttpRequest::setStatus(MyHttpRequest::RequestStatus new_status)
{
    if( new_status!=m_status ) {
        m_status = new_status;
        emit statusChanged();
    }
}

const NetworkAccessManager *MyHttpRequest::getNetworkAccessManager()
{
    return manager;
}

QNetworkRequest* MyHttpRequest::getNetworkRequest()
{
    return &request;
}

void MyHttpRequest::finished(QNetworkReply *reply)
{
    Data temp = queue_replyData.dequeue ();
    ReplyType type=temp.replyType;
    bool isError = !(reply->error () == QNetworkReply::NoError);
    
    if(type==CallbackFun){
        QScriptValueList list;
        list.append (QScriptValue(isError));
        list.append (QScriptValue(isError?reply->errorString ():reply->readAll ()));//
        temp.callbackFun.call (QScriptValue(), list);
    }else if(type==ConnectSlot){
        QObject* obj = temp.caller;
        QByteArray slotName = temp.slotName;
        
        if(obj!=NULL){
            bool ok;
            int parameterCount = obj->metaObject()->method(obj->metaObject()->indexOfMethod(slotName)).parameterTypes().length();
            if(parameterCount==1){//如果形参个数为个
                ok = QMetaObject::invokeMethod(obj, slotName, Q_ARG(QNetworkReply*, reply));
            }else{
                ok = QMetaObject::invokeMethod(obj, slotName, Q_ARG(QVariant, isError), Q_ARG(QVariant, (isError?reply->errorString ():reply->readAll ())));
            }
            if(!ok){
                qDebug()<<"MyHttpRequest:调用槽"+slotName+"失败";
            }
        }
    }
    send();//继续请求
}

void MyHttpRequest::send(QScriptValue callbackFun, QUrl url, QByteArray data, bool highRequest/*=false*/)
{
    if((!callbackFun.isFunction())||url.toString ()=="")
        return;
    if(highRequest){
        new MyHttpRequestPrivate(request, callbackFun, url, data);//进行高优先级的网络请求
    }else {
        requestData request_data;
        request_data.url = url;
        request_data.data = data;
        queue_requestData<<request_data;
        
        Data temp;
        temp.callbackFun = callbackFun;
        temp.replyType = CallbackFun;
        queue_replyData<<temp;
        if(status ()==Idle){
            send();
        }
    }
}

void MyHttpRequest::get(QObject *caller, QByteArray slotName, QUrl url, bool highRequest/*=false*/)
{
    send(caller, slotName, url, "", highRequest);
}

void MyHttpRequest::post(QObject *caller, QByteArray slotName, QUrl url, QByteArray data, bool highRequest/*=false*/)
{
    send(caller, slotName, url, data, highRequest);
}

void MyHttpRequest::get(QScriptValue callbackFun, QUrl url, bool highRequest/*=false*/)
{
    send (callbackFun, url, "", highRequest);
}

void MyHttpRequest::post(QScriptValue callbackFun, QUrl url, QByteArray data, bool highRequest/*=false*/)
{
    send (callbackFun, url, data, highRequest);
}

void MyHttpRequest::send(QObject *caller, QByteArray slotName, QUrl url, QByteArray data, bool highRequest/*=false*/)
{
    if(caller==NULL||slotName==""||url.toString ()=="")
        return;
    if(highRequest){
        new MyHttpRequestPrivate(request, caller, slotName, url, data);//进行高优先级的网络请求
    }else{
        QRegExp reg("[_A-Za-z][_A-Za-z0-9]*");//提取函数名
        if(reg.indexIn (slotName)>=0){
            requestData request_data;
            request_data.url = url;
            request_data.data = data;
            queue_requestData<<request_data;
            
            Data temp;
            temp.caller = caller;
            slotName = reg.cap (0).toLatin1 ();
            temp.slotName = slotName;
            temp.replyType = ConnectSlot;
            queue_replyData<<temp;
            if(status ()==Idle){
                send();
            }
        }else{
            qDebug()<<"MyHttpRequest:"<<slotName<<"不是一个规范的函数，所以不可能是一个槽";
        }
    }
}

void MyHttpRequest::abort()
{
    if(!m_reply.isNull ()){
        m_reply->abort ();
    }
}

void MyHttpRequest::send()
{
    //qDebug()<<queue_replyData.count ()<<queue_requestData.count ()<<QThread::currentThread ();
    if( queue_requestData.count ()>0){
        setStatus (Busy);
        requestData temp = queue_requestData.dequeue ();
        request.setUrl (temp.url);
        QByteArray data = temp.data;
        if( data=="" )
            m_reply = manager->get (request);
        else
            m_reply = manager->post (request, data);
    }else
        setStatus (Idle);//设置状态为空闲
}

QString MyHttpRequest::errorString()
{
    return m_reply->errorString ();
}

MyHttpRequestPrivate::MyHttpRequestPrivate(QNetworkRequest request, QScriptValue callbackFun, QUrl url, QByteArray data):
    MyHttpRequest(0)
{
    this->request = request;
    send(callbackFun, url, data, false);
}

MyHttpRequestPrivate::MyHttpRequestPrivate(QNetworkRequest request, QObject *caller, QByteArray slotName, QUrl url, QByteArray data):
    MyHttpRequest(0)
{
    this->request = request;
    send(caller, slotName, url, data, false);
}

void MyHttpRequestPrivate::finished(QNetworkReply *reply)
{
    MyHttpRequest::finished (reply);
    deleteLater ();//销毁自己
}
