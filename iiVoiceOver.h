#pragma once

#include <QString>
#include <QtGlobal>

#if defined(IIVOICEOVER_BUILDING_LIBRARY)
#  define IIVOICEOVER_EXPORT Q_DECL_EXPORT
#else
#  define IIVOICEOVER_EXPORT Q_DECL_IMPORT
#endif

namespace iiVoiceOver {

[[nodiscard]] IIVOICEOVER_EXPORT QString helloWorld();

} // namespace iiVoiceOver
