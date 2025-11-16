#pragma once

#include <QString>

/*! \brief UiUtils命名空间提供QSS加载辅助函数。 */
namespace UiUtils {
/*! \brief 从Qt资源路径中读取QSS内容。
 *  \param resourcePath Qt资源路径，例如":/ui/qss/ProfileDialog.qss"。
 *  \return UTF-8文本内容，失败时返回空字符串。
 */
QString loadStyleSheet(const QString &resourcePath);
}
