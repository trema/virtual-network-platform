#
# Copyright (C) 2013 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

require 'db/base'
require 'db/mac-learning'
require 'db/slice-state'

module DB

  class Slice < Base
    self.table_name = "slices"

    def state
      SliceState.new read_attribute( :state )
    end

    def state= ( value )
      write_attribute( :state, value.to_i )
    end

    def mac_learning
      MacLearning.new read_attribute( :mac_learning )
    end

    def mac_learning= ( value )
      if value.nil?
        value = MAC_LEARNING_DISABLE
      end
      write_attribute( :mac_learning, value.to_i )
    end

    def self.generate_id
      begin
        for i in 1..10
          id = rand( 16777216 )
          find( id )
        end
        raise StandardError.new
      rescue ActiveRecord::RecordNotFound
        id
      end
    end

  end

end
