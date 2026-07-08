# common/CMakeLists.txt

project(bookclub_common)

# ---- Collect Sources ----
set(COMMON_SOURCES
    Models/UserAccount.cpp
    Models/Admin.cpp
    Models/Publisher.cpp
    Models/RegularUser.cpp
    Models/Book.cpp
    Models/Cart.cpp
    Models/CartItem.cpp
    Models/Order.cpp
    Models/Review.cpp
    Models/Notification.cpp
    Models/LibraryShelf.cpp
    Models/UserLibrary.cpp
    Models/Discount.cpp
    Models/Genre.cpp
    Models/StudySession.cpp
    Models/PublisherStats.cpp
    Network/Protocol.cpp
    Network/Message.cpp
    Network/PacketParser.cpp
    Network/Serializer.cpp

    Utils/IdGenerator.cpp
    Utils/Logger.cpp
    Utils/CryptoUtils.cpp
    Utils/PasswordHasher.cpp
    Utils/DateTimeUtils.cpp
    Utils/FileSystemUtils.cpp
    Utils/ValidationUtils.cpp
    Config/AppConfig.cpp
)

set(COMMON_HEADERS
    Models/UserAccount.h
    Models/Admin.h
    Models/Publisher.h
    Models/RegularUser.h
    Models/Book.h
    Models/BookFilter.h
    Models/Cart.h
    Models/CartItem.h
    Models/Order.h
    Models/Review.h
    Models/Notification.h
    Models/LibraryShelf.h
    Models/UserLibrary.h
    Models/Discount.h
    Models/Genre.h
    Models/SearchQuery.h
    Models/ServerHealth.h
    Models/StudySession.h
    Models/PublisherStats.h
    Network/Protocol.h
    Network/Message.h
    Network/PacketParser.h
    Network/Serializer.h

    Utils/IdGenerator.h
    Utils/Logger.h
    Utils/CryptoUtils.h
    Utils/PasswordHasher.h
    Utils/DateTimeUtils.h
    Utils/FileSystemUtils.h
    Utils/ValidationUtils.h
    Config/AppConfig.h
    Interfaces/IAuthService.h
    Interfaces/IBookRepository.h
    Interfaces/IBookService.h
    Interfaces/INotificationRepository.h
    Interfaces/IOrderRepository.h
    Interfaces/IReviewRepository.h
    Interfaces/IUserRepository.h
    AppEnums.h
    Constants.h
    Result.h
)

# ---- Build Static Library ----
add_library(bookclub_common STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})

# ---- Link Qt5 ----
target_link_libraries(bookclub_common
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)

# ---- Include Directories ----
target_include_directories(bookclub_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
