#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QVector>
#include <QByteArray>
#include <QtGlobal>

enum MsgType : quint32 {
    TEXT   = 0,
    IMAGE  = 1,
    MSGFILE= 2,  // 避免与 C 库 FILE 冲突
    SCREEN = 3
};

#pragma pack(push, 1)
struct ProtocolHeader {
    quint32 userId;
    quint64 msgId;
    quint32 type;
    quint32 totalLength;
    quint32 seq;
    quint32 totalSeq;
    quint32 payloadLen;
};
#pragma pack(pop)

const int MAX_CHUNK_SIZE = 64 * 1024;

inline QVector<QByteArray> makeFragments(
    quint32 userId, quint64 msgId, quint32 type, const QByteArray &data)
{
    QVector<QByteArray> packets;
    int totalLength = data.size();
    int totalSeq = (totalLength + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;
    for (int seq = 0; seq < totalSeq; ++seq) {
        int offset = seq * MAX_CHUNK_SIZE;
        int chunkLen = qMin(MAX_CHUNK_SIZE, totalLength - offset);
        ProtocolHeader header;
        header.userId      = userId;
        header.msgId       = msgId;
        header.type        = type;
        header.totalLength = totalLength;
        header.seq         = seq;
        header.totalSeq    = totalSeq;
        header.payloadLen  = chunkLen;
        QByteArray packet;
        packet.append(reinterpret_cast<const char*>(&header), sizeof(header));
        packet.append(data.constData() + offset, chunkLen);
        packets.append(packet);
    }
    return packets;
}

#endif // PROTOCOL_H
