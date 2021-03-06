#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

IF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # quickstart
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ru.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=ru -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ru.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ru.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ru.txt
    COMMENT "Building weechat_quickstart.ru.html"
  )
  ADD_CUSTOM_TARGET(doc-quickstart-ru ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ru.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ru.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

ENDIF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)
