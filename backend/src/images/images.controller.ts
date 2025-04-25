import { Controller, Post, UseInterceptors, UploadedFile, Get, Res, Param } from '@nestjs/common';
import { FileInterceptor } from '@nestjs/platform-express';
import { Response } from 'express';
import { ImagesService } from './images.service';
import { diskStorage } from 'multer';
import { extname } from 'path';
import * as fs from 'fs';

@Controller('images')
export class ImagesController {
  constructor(private readonly imagesService: ImagesService) {}

  @Post('upload')
  @UseInterceptors(
    FileInterceptor('image', {
      storage: diskStorage({
        destination: './uploads',
        filename: (req, file, callback) => {
          const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1e9);
          const ext = extname(file.originalname) || '.jpg'; // Default to .jpg if no extension
          callback(null, `image-${uniqueSuffix}${ext}`);
        },
      }),
      fileFilter: (req, file, callback) => {
        // Check if file is an image
        if (!file.mimetype.includes('image')) {
          return callback(new Error('Only image files are allowed!'), false);
        }
        callback(null, true);
      },
      limits: {
        fileSize: 10 * 1024 * 1024, // 10MB max file size
      },
    }),
  )
  async uploadImage(@UploadedFile() file: Express.Multer.File) {
    console.log('Received file:', file);
    
    if (!file) {
      return { success: false, message: 'No file uploaded' };
    }
    
    const savedImage = await this.imagesService.saveImage({
      filename: file.filename,
      path: file.path,
      mimetype: file.mimetype,
      size: file.size,
      timestamp: new Date(),
    });
    
    return {
      success: true,
      message: 'Image uploaded successfully',
      image: savedImage,
    };
  }

  @Get('list')
  async getImages() {
    return this.imagesService.getAllImages();
  }

  @Get(':filename')
  getImageFile(@Param('filename') filename: string, @Res() res: Response) {
    return this.imagesService.getImageByFilename(filename, res);
  }
} 