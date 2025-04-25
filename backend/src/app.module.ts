import { Module } from '@nestjs/common';
import { ImagesController } from './images/images.controller';
import { ImagesService } from './images/images.service';

@Module({
  imports: [],
  controllers: [ImagesController],
  providers: [ImagesService],
})
export class AppModule {} 