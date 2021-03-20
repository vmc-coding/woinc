#!/bin/sh

for i in $*; do
    tmp=$(mktemp)
    cat <<EOF >${tmp}
/* $i --
   Written and Copyright (C) $(date '+%Y') by ${USER}.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

EOF
    cat ${i} >> ${tmp}
    mv ${tmp} ${i}
done

