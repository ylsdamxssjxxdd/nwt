#pragma once

#include <QPixmap>
#include <QString>

struct ProfileDetails;

/*!
 * \brief AvatarHelper �ṩͳһ�������û�ͷ������
 */
namespace AvatarHelper {

/*!
 * \brief AvatarDescriptor ����ͷ������Ҫ����Դ
 */
struct AvatarDescriptor {
    QString displayName;
    QString gender;
    QString avatarPath;
};

/*!
 * \brief descriptorFromProfile ���ݸ���档������ͷ���������
 * \param profile ��ǰ�û�����������ϸ
 * \return ����ͷ������Ϣ
 */
AvatarDescriptor descriptorFromProfile(const ProfileDetails &profile);

/*!
 * \brief createAvatarPixmap ����ͷ��ͼ��
 * \param descriptor ��������Ϣ
 * \param size ����ͼƬ�ĳߴ�
 * \return ��Բ�ζ���ͼ��
 */
QPixmap createAvatarPixmap(const AvatarDescriptor &descriptor, int size);

} // namespace AvatarHelper

